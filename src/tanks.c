#include <rp6502.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "constants.h"
#include "tanks.h"
#include "player.h"
#include "enemybase.h"
#include "hostages.h"

// Demo mode
extern bool is_demo_mode;

// --- TANK STATE ---
bool tanks_triggered = false; // Have we collected 4 hostages yet?

Tank tanks[NUM_TANKS];

// Initial Spawn Locations (Example)
const int32_t TANK_SPAWNS[NUM_TANKS] = {
    2000L << SUBPIXEL_BITS,
    3000L << SUBPIXEL_BITS
};

// Spawn Timer
static int tank_spawn_timer = 0;

int get_closest_base_index(void) {
    int closest_i = -1;
    int32_t min_dist = 0x7FFFFFFF;

    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        // REMOVED: if (base_state[i].destroyed) continue; 
        
        int32_t base_x = ENEMY_BASE_LOCATIONS[i];
        int32_t dist = labs(chopper_world_x - base_x);
        
        if (dist < min_dist) {
            min_dist = dist;
            closest_i = i;
        }
    }
    return closest_i;
}

// Helper to get pointer to specific 16x16 tile index
uint16_t get_tank_tile_ptr(int index) {
    return TANK_DATA + (index * 128);
}

void reset_tanks(void) {
    // 1. Reset Global Logic
    tanks_triggered = false;
    tank_spawn_timer = 0;

    // 2. Clear Active Tanks & Sprites
    for (int t = 0; t < NUM_TANKS; t++) {
        tanks[t].active = false;
        
        // Hide all 9 sprites for this tank
        for (int s = 0; s < 9; s++) {
            unsigned cfg = TANK_CONFIG + ((t * 9 + s) * sizeof(vga_mode4_sprite_t));
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
        }
    }
}

void update_tanks(void) {

    // --- DEBUG: TRACK INVENTORY ---
    // static int debug_print_timer = 0;
    // debug_print_timer++;
    
    // if (debug_print_timer > 60) { // Print once per second
    //     debug_print_timer = 0;
        
    //     int b = get_closest_base_index();
    //     int active_count = 0;
        
    //     // Count how many tanks are actually driving around right now
    //     for(int t=0; t<NUM_TANKS; t++) {
    //         if (tanks[t].active) active_count++;
    //     }

    //     if (b != -1) {
    //         printf("DEBUG - Base %d | Garage: %d | Cooldown: %d | Active Tanks: %d | Triggered: %d\n", 
    //                b, 
    //                base_state[b].tanks_remaining, 
    //                base_state[b].tank_cooldown, 
    //                active_count,
    //                tanks_triggered);
    //     }
    // }

    // =========================================================
    // 0. UPDATE BASE COOLDOWNS (The Missing Piece)
    // =========================================================
    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        if (base_state[i].tank_cooldown > 0) {
            base_state[i].tank_cooldown--;
        }
    }
    
    // =========================================================
    // 1. GLOBAL TRIGGER CHECK
    // =========================================================
    int total_progress = hostages_rescued_count + hostages_on_board;
    
    if (!tanks_triggered) {
        if ((total_progress >= TANK_SPAWN_TRIGGER) || is_demo_mode) {
            tanks_triggered = true;
            // FILL ALL GARAGES
            for (int i = 0; i < NUM_ENEMY_BASES; i++) {
                base_state[i].tanks_remaining = TANKS_PER_BASE;
                base_state[i].tank_cooldown = 0;
            }
        }
    }

    // =========================================================
    // 2. SPAWN LOGIC (Closest Base Only)
    // =========================================================
    if (tank_spawn_timer > 0) tank_spawn_timer--;

    int b = get_closest_base_index();

    // Only try to spawn if Triggered, Cooldown Ready, and Tanks Available
    if (tanks_triggered && b != -1 && 
        base_state[b].tanks_remaining > 0 && 
        base_state[b].tank_cooldown == 0 && 
        tank_spawn_timer == 0) {
        
        // Find hardware slot
        int free_slot = -1;
        for (int t = 0; t < NUM_TANKS; t++) {
            if (!tanks[t].active) {
                free_slot = t;
                break;
            }
        }

        if (free_slot != -1) {
            int32_t base_x = ENEMY_BASE_LOCATIONS[b];
            
            // Check currently active tanks for THIS base to find the empty side
            bool has_left_tank = false;
            bool has_right_tank = false;

            for (int t = 0; t < NUM_TANKS; t++) {
                if (tanks[t].active && tanks[t].base_id == b) {
                    if (tanks[t].world_x < base_x) has_left_tank = true;
                    else has_right_tank = true;
                }
            }

            // Decide where to spawn
            int32_t spawn_x = 0;
            bool can_spawn = false;
            int8_t start_dir = 0;

            // Priority: Fill Left, then Fill Right
            if (!has_left_tank) {
                // Try Left Spawn
                spawn_x = base_x - TANK_SPAWN_DIST;
                start_dir = 1; // Drive Right
                can_spawn = true;
            } 
            else if (!has_right_tank) {
                // Try Right Spawn
                spawn_x = base_x + TANK_SPAWN_DIST;
                start_dir = -1; // Drive Left
                can_spawn = true;
            }

            // VISIBILITY CHECK
            // If the calculated spawn point is on screen, ABORT.
            if (can_spawn) {
                // Screen range: CameraX to CameraX + 320
                // We add a small buffer (32px) to ensure it's fully offscreen
                int32_t screen_left = camera_x - OFFSCREEN_BUFFER;
                int32_t screen_right = camera_x + SCREEN_WIDTH_SUB + OFFSCREEN_BUFFER;

                if (spawn_x > screen_left && spawn_x < screen_right) {
                    can_spawn = false; // Player is looking at the spawn point!
                }
            }

            // FINAL EXECUTION
            if (can_spawn) {
                // Clamp World Limits
                if (spawn_x < WORLD_MIN_X_SUB) spawn_x = WORLD_MIN_X_SUB;
                if (spawn_x > WORLD_MAX_X_SUB) spawn_x = WORLD_MAX_X_SUB;

                tanks[free_slot].active = true;
                tanks[free_slot].base_id = b;
                tanks[free_slot].world_x = spawn_x;
                tanks[free_slot].y = GROUND_Y_SUB + (32 << SUBPIXEL_BITS); // Your Y coord
                tanks[free_slot].direction = start_dir;
                tanks[free_slot].health = 1;
                
                base_state[b].tanks_remaining--;
                tank_spawn_timer = 60; // Wait 1 second before trying next spawn
                tanks[free_slot].fire_cooldown = 20; // 60 + (rand() % 60);
            }
        }
    }

    // =========================================================
    // 3. AI & MOVEMENT LOOP
    // =========================================================
    for (int t = 0; t < NUM_TANKS; t++) {
        // --- CLEANUP ---
        if (!tanks[t].active) {
            // Hide sprites
            for(int s=0; s<9; s++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + s) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
            }
            continue;
        }

        // --- DESPAWN LOGIC ---
        // If tank is from a different base (we moved away), remove it.
        if (tanks[t].base_id != get_closest_base_index()) {
            tanks[t].active = false;
            base_state[tanks[t].base_id].tanks_remaining++; // Return to garage
            continue;
        }

        // --- AI LOGIC ---
        int32_t base_x = ENEMY_BASE_LOCATIONS[tanks[t].base_id];
        int32_t dist_from_base = tanks[t].world_x - base_x;
        int32_t chop_cx = chopper_world_x + (16 << SUBPIXEL_BITS);
        int32_t tank_cx = tanks[t].world_x + (20 << SUBPIXEL_BITS); 
        int32_t dist_to_chopper = chop_cx - tank_cx;

        // 1. Basic Desire
        int8_t target_dir = (dist_to_chopper > 0) ? 1 : -1;

        // 2. Leash Override (100px from base)
        if (dist_from_base > TANK_LEASH_DIST)      target_dir = -1; 
        else if (dist_from_base < -TANK_LEASH_DIST) target_dir = 1;

        // 3. Stop if under chopper
        if (labs(dist_to_chopper) < (4 << SUBPIXEL_BITS)) target_dir = 0;

        // 4. Staggering (Prevent Overlap)
        if (target_dir != 0) {
            for (int other = 0; other < NUM_TANKS; other++) {
                if (t == other || !tanks[other].active) continue;
                int32_t sep = tanks[other].world_x - tanks[t].world_x;
                if (target_dir == 1 && sep > 0 && sep < TANK_SPACING) target_dir = 0;
                if (target_dir == -1 && sep < 0 && sep > -TANK_SPACING) target_dir = 0;
            }
        }

        // --- PHYSICS & ANIMATION ---
        tanks[t].direction = target_dir;
        if (target_dir == 1)  tanks[t].world_x += TANK_SPEED;
        if (target_dir == -1) tanks[t].world_x -= TANK_SPEED;

        if (target_dir != 0) {
            tanks[t].anim_timer++;
            if (tanks[t].anim_timer > 8) {
                tanks[t].anim_timer = 0;
                tanks[t].anim_frame = !tanks[t].anim_frame;
            }
        }

        // --- TURRET AIMING ---
        int16_t diff_x_px = dist_to_chopper >> SUBPIXEL_BITS;
        if (diff_x_px > 20)      tanks[t].turret_dir = TURRET_RIGHT;
        else if (diff_x_px < -20) tanks[t].turret_dir = TURRET_UP_LEFT;
        else                      tanks[t].turret_dir = TURRET_UP;

        // --- RENDER ---
        int32_t screen_sub = tanks[t].world_x - camera_x;
        int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

        if (screen_px > -100 && screen_px < 340) {
            // (Your existing Body/Turret render code goes here)
            // ...
            // Re-use the render block from previous steps
            // ...
            int body_start = (tanks[t].anim_frame == 0) ? 0 : 5;
            int16_t base_y_px = tanks[t].y >> SUBPIXEL_BITS;

            for (int i = 0; i < 5; i++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + i) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, (screen_px + (i * 8)));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, base_y_px);
                xram0_struct_set(cfg, vga_mode4_sprite_t, xram_sprite_ptr, get_tank_tile_ptr(body_start + i));
            }

            int turret_start;
            switch(tanks[t].turret_dir) {
                case TURRET_UP_LEFT: turret_start = 10; break;
                case TURRET_UP:      turret_start = 14; break;
                default:             turret_start = 18; break; 
            }

            for (int i = 0; i < 4; i++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + (5+i)) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, (screen_px + 4 + (i * 8)));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, (base_y_px - 8));
                xram0_struct_set(cfg, vga_mode4_sprite_t, xram_sprite_ptr, get_tank_tile_ptr(turret_start + i));
            }
        } else {
            // Offscreen hide
            for(int s=0; s<9; s++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + s) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
            }
        }
    }
}