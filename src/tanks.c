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

void update_tanks(void) {

    int closest_base_idx = get_closest_base_index();

    // --- 1. UPDATE BASE COOLDOWNS ---
    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        if (base_state[i].tank_cooldown > 0) {
            base_state[i].tank_cooldown--;
        }
    }

    // --- 2. SPAWN LOGIC ---
    int total_progress = hostages_rescued_count + hostages_on_board;
    if (tank_spawn_timer > 0) tank_spawn_timer--;

    if (total_progress >= TANK_SPAWN_TRIGGER && tank_spawn_timer == 0) {
        
        int free_slot = -1;
        for (int t = 0; t < NUM_TANKS; t++) {
            if (!tanks[t].active) {
                free_slot = t;
                break;
            }
        }

        int b = get_closest_base_index();

        if (free_slot != -1 && b != -1 && 
            base_state[b].tanks_remaining > 0 && 
            base_state[b].tank_cooldown == 0) {
            
            int32_t base_x = ENEMY_BASE_LOCATIONS[b];
            int32_t spawn_x = 0;
            int8_t  start_dir = 0;

            // --- SMARTER FLANK LOGIC ---
            // Default: Left Flanker (Drive Right)
            bool spawn_left_side = true; 

            // If there is ALREADY 1 tank out there, check which one it is
            // so we can spawn the opposite.
            if (base_state[b].tanks_remaining == 1) {
                for (int t = 0; t < NUM_TANKS; t++) {
                    if (tanks[t].active && tanks[t].base_id == b) {
                        // If the existing tank is driving Right (Left Flanker),
                        // we should spawn the Right Flanker (False).
                        // If existing is driving Left (Right Flanker), 
                        // we spawn Left Flanker (True).
                        
                        // We use the tank's CURRENT position relative to base to be sure
                        if (tanks[t].world_x < base_x) {
                            spawn_left_side = false; // Existing is Left, Spawn Right
                        } else {
                            spawn_left_side = true;  // Existing is Right, Spawn Left
                        }
                        break;
                    }
                }
            }
            // If tanks_remaining == 2, we default to Left Side (True) to start the pair.

            if (spawn_left_side) {
                // LEFT FLANKER (Spawns Left, Drives Right)
                int32_t ideal_x = base_x - TANK_SPAWN_OFFSET_X;
                if (ideal_x > camera_x) spawn_x = camera_x - OFFSCREEN_BUFFER;
                else spawn_x = ideal_x;
                start_dir = 1; 
            } 
            else {
                // RIGHT FLANKER (Spawns Right, Drives Left)
                int32_t ideal_x = base_x + TANK_SPAWN_OFFSET_X;
                int32_t camera_right = camera_x + SCREEN_WIDTH_SUB;
                if (ideal_x < camera_right) spawn_x = camera_right + OFFSCREEN_BUFFER;
                else spawn_x = ideal_x;
                start_dir = -1;
            }

            // Clamp
            if (spawn_x < WORLD_MIN_X_SUB) spawn_x = WORLD_MIN_X_SUB;
            if (spawn_x > WORLD_MAX_X_SUB) spawn_x = WORLD_MAX_X_SUB;

            // Activate
            tanks[free_slot].active = true;
            tanks[free_slot].base_id = b;
            tanks[free_slot].world_x = spawn_x;
            tanks[free_slot].y = GROUND_Y_SUB + (32 << SUBPIXEL_BITS);
            tanks[free_slot].direction = start_dir;
            tanks[free_slot].health = 1;
            
            base_state[b].tanks_remaining--;
            tank_spawn_timer = 60; 
        }
    }


    
    for (int t = 0; t < NUM_TANKS; t++) {
        if (!tanks[t].active) {
            // Hide all 9 sprites for this tank
            for(int s=0; s<9; s++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + s) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
            }
        continue;
        }

        // AUTO-RECYCLE: If tank is far away AND belongs to a different base
        // Return it to the garage so it can respawn at the new base.
        int32_t dist_to_player = labs(tanks[t].world_x - chopper_world_x);
        
        if (dist_to_player > TANK_DESPAWN_DIST && tanks[t].base_id != closest_base_idx) {
            tanks[t].active = false;
            base_state[tanks[t].base_id].tanks_remaining++; // Return to old base inventory
            continue; // Skip rest of loop (will be hidden next frame)
        }

        // --- AI LOGIC ---
        int32_t base_x = ENEMY_BASE_LOCATIONS[tanks[t].base_id];
        int32_t dist_from_base = tanks[t].world_x - base_x;
        int32_t chop_cx = chopper_world_x + (16 << SUBPIXEL_BITS);
        int32_t tank_cx = tanks[t].world_x + (20 << SUBPIXEL_BITS); // Width 40, Center 20
        int32_t dist_to_chopper = chop_cx - tank_cx;

        // 1. Basic Desire
        int8_t target_dir = (dist_to_chopper > 0) ? 1 : -1;

        // 2. Leash (100px)
        if (dist_from_base > TANK_LEASH_DIST)      target_dir = -1; 
        else if (dist_from_base < -TANK_LEASH_DIST) target_dir = 1;

        // 3. Stop if under chopper
        if (labs(dist_to_chopper) < (4 << SUBPIXEL_BITS)) target_dir = 0;

        // 4. STAGGERING (Prevent Overlap)
        if (target_dir != 0) {
            for (int other = 0; other < NUM_TANKS; other++) {
                if (t == other || !tanks[other].active) continue;

                int32_t sep = tanks[other].world_x - tanks[t].world_x;
                
                // If moving Right and someone is < 40px ahead
                if (target_dir == 1 && sep > 0 && sep < TANK_SPACING) {
                    target_dir = 0; // Wait
                }
                // If moving Left and someone is < 40px ahead (negative diff)
                if (target_dir == -1 && sep < 0 && sep > -TANK_SPACING) {
                    target_dir = 0; // Wait
                }
            }
        }

        // --- PHYSICS ---
        tanks[t].direction = target_dir;
        if (target_dir == 1)  tanks[t].world_x += TANK_SPEED;
        if (target_dir == -1) tanks[t].world_x -= TANK_SPEED;

        // --- ANIMATION ---
        if (target_dir != 0) {
            tanks[t].anim_timer++;
            if (tanks[t].anim_timer > 8) {
                tanks[t].anim_timer = 0;
                tanks[t].anim_frame = !tanks[t].anim_frame;
            }
        }


        // ---------------------------------------------------
        // 2. TURRET AIMING
        // ---------------------------------------------------
        // Compare Tank Center X with Chopper Center X
        
        // Calculate difference in pixels
        int16_t diff_x = (chop_cx - tank_cx) >> SUBPIXEL_BITS;
        int16_t diff_y = (chopper_y - tanks[t].y) >> SUBPIXEL_BITS; // negative means chopper is above

        if (diff_x > 20) {
            tanks[t].turret_dir = TURRET_RIGHT; // Player is to the Right
        } 
        else if (diff_x < -20) {
            // Player is to the Left
            // We only have Up-Left, Up, Right. Use Up-Left.
            tanks[t].turret_dir = TURRET_UP_LEFT; 
        } 
        else {
            // Player is roughly overhead
            tanks[t].turret_dir = TURRET_UP;
        }

        // ---------------------------------------------------
        // 3. RENDER (THE COMPOSITE SPRITE)
        // ---------------------------------------------------
        int32_t screen_sub = tanks[t].world_x - camera_x;
        int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

        // Culling Check (Tank is 40px wide)
        if (screen_px > -100 && screen_px < 340) {
            
            // --- DRAW BODY (Sprites 0-4) ---
            // Indices: Frame 0 (0-4), Frame 1 (5-9)
            int body_start_idx = (tanks[t].anim_frame == 0) ? 0 : 5;
            int16_t base_y_px = tanks[t].y >> SUBPIXEL_BITS;

            for (int i = 0; i < 5; i++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + i) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, (screen_px + (i * 8)));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, base_y_px);
                xram0_struct_set(cfg, vga_mode4_sprite_t, xram_sprite_ptr, get_tank_tile_ptr(body_start_idx + i));
            }

            // --- DRAW TURRET (Sprites 5-8) ---
            // Width: 4 sprites (64px). 
            // Centered on Body (80px): Offset X = 8px.
            // Height: On top of body: Offset Y = -16px.
            
            int turret_start_idx;
            switch(tanks[t].turret_dir) {
                case TURRET_UP_LEFT: turret_start_idx = 10; break;
                case TURRET_UP:      turret_start_idx = 14; break;
                default:             turret_start_idx = 18; break; // Right
            }

            for (int i = 0; i < 4; i++) {
                // Config indices 5,6,7,8 for this tank
                unsigned cfg = TANK_CONFIG + ((t*9 + (5+i)) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, (screen_px + 4 + (i * 8)));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, (base_y_px - 8));
                xram0_struct_set(cfg, vga_mode4_sprite_t, xram_sprite_ptr, get_tank_tile_ptr(turret_start_idx + i));
            }

        } else {
            // Off-screen: Hide all 9 sprites
            for(int s=0; s<9; s++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + s) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
            }
        }
    }
}