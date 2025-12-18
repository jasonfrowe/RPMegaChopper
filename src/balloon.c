#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "constants.h"
#include "player.h"
#include "input.h" // For globals like camera_x
#include "balloon.h"
#include "hostages.h"
#include "enemybase.h"
#include "explosion.h"
#include "homebase.h"
#include "sound.h"


#define BALLOON_GROUND_Y        (GROUND_Y_SUB + (12 << SUBPIXEL_BITS)) // Ground level for balloon crash

Balloon balloon; 

// Sprite Data Offsets (16x16 = 512 bytes)
// Frame 0: Bottom=0, Top=512
// Frame 1: Bottom=1024, Top=1536
// Frame 2: Bottom=2048, Top=2560
uint16_t get_balloon_ptr(int frame, int part) {
    // part 0 = Bottom, part 1 = Top
    int base_idx = frame * 2; 
    return BALLOON_DATA + ((base_idx + part) * 512);
}

void reset_balloon(void) {
    balloon.active = false;
    balloon.respawn_timer = 0; // Reset so it spawns when criteria are met
    
    // Hide Sprites
    xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
}


void update_balloon(void) {
    int total_progress = hostages_total_spawned;

    // =========================================================
    // 1. SPAWN / RESPAWN LOGIC
    // =========================================================
    if (!balloon.active) {
        if (balloon.respawn_timer > 0) {
            balloon.respawn_timer--;
            
            // HIDE SPRITES while waiting
            xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
            xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
            return;
        }

        // --- PROGRESS CHECK ---
        // Only start hunting after 16 hostages have appeared
        if (total_progress >= 16) {
            
            // --- DEFINE BOUNDARIES ---
            int32_t safety_line = (ENEMY_BASE_LOCATIONS[3] + HOMEBASE_WORLD_X) / 2;
            int32_t offscreen_margin = 64L << SUBPIXEL_BITS; // Spawn 64px off screen

            // --- CALCULATE CANDIDATES ---
            int32_t camera_left_edge  = camera_x;
            int32_t camera_right_edge = camera_x + (320L << SUBPIXEL_BITS);

            int32_t spawn_left_opt  = camera_left_edge - offscreen_margin;
            int32_t spawn_right_opt = camera_right_edge + offscreen_margin;

            // --- PICK A SIDE ---
            int32_t chosen_x = 0;
            
            // 50/50 Chance, OR biased by player movement? 
            // Let's go random for unpredictability.
            bool try_right = (rand() % 2 == 0);

            if (try_right) {
                // Try spawning ahead/behind on the right
                if (spawn_right_opt < safety_line) {
                    chosen_x = spawn_right_opt;
                } else {
                    // Right side is blocked by Home Base safety zone. Force Left.
                    chosen_x = spawn_left_opt;
                }
            } else {
                // Try spawning on the left
                if (spawn_left_opt > (WORLD_MIN_X_SUB)) {
                    chosen_x = spawn_left_opt;
                } else {
                    // Left side is blocked by World Edge. Force Right.
                    // (But check safety line again to be sure)
                    if (spawn_right_opt < safety_line) {
                        chosen_x = spawn_right_opt;
                    } else {
                        return; // Nowhere valid to spawn! (Player is squeezed in a corner?)
                    }
                }
            }

            // --- FINAL VISIBILITY CHECK ---
            // Ensure we didn't get clamped onto the screen
            int32_t screen_test = chosen_x - camera_x;
            int16_t screen_px = screen_test >> SUBPIXEL_BITS;

            if (screen_px < -32 || screen_px > 350) {
                // SPAWN!
                balloon.active = true;
                balloon.is_falling = false; // Reset falling state
                balloon.world_x = chosen_x;
                balloon.y = GROUND_Y_SUB - (60 << SUBPIXEL_BITS); // High altitude
                balloon.vx = 0;
                balloon.vy = 0;
                balloon.anim_frame = 0;
            }
        }
        return; 
    }

    // --- CASE A: FALLING (Shot Down) ---
    if (balloon.is_falling) {
        // 1. Apply Gravity
        balloon.vy += 2; // Gravity (0.125 px/frame)
        balloon.y += balloon.vy;
        
        // 2. Apply Momentum (optional, usually 0)
        balloon.world_x += balloon.vx;

        // 3. Fast Animation (Spinning/Flailing)
        balloon.anim_timer++;
        if (balloon.anim_timer > 2) { // Very fast (every 2 frames)
            balloon.anim_timer = 0;
            balloon.anim_frame++;
            if (balloon.anim_frame > 2) balloon.anim_frame = 0;
        }

        // 4. Ground Impact Check
        if (balloon.y >= BALLOON_GROUND_Y) {
            // CRASH!
            trigger_explosion(balloon.world_x, BALLOON_GROUND_Y);
            sfx_explosion_small(); // crash sound
            
            // Deactivate and start Respawn Timer
            balloon.active = false;
            balloon.is_falling = false;
            balloon.respawn_timer = BALLOON_RESPAWN;
            return; // Stop processing
        }
    }
    // --- CASE B: FLYING (Normal AI) ---
    else {

        // =========================================================
        // 2. AI: HOMING
        // =========================================================
        
        // --- Define Boundaries ---
        // Safety Line: Halfway between the last enemy base (Index 3) and Home
        int32_t safety_line = (ENEMY_BASE_LOCATIONS[3] + HOMEBASE_WORLD_X) / 2;
        
        // Floor Ceiling: 32 pixels above ground
        int32_t min_altitude = GROUND_Y_SUB - (32 << SUBPIXEL_BITS);
        
        // Target: Chopper Center
        int32_t target_x = chopper_world_x + (16 << SUBPIXEL_BITS);
        int32_t target_y = chopper_y + (8 << SUBPIXEL_BITS);

        // --- X Movement ---
        // If balloon tries to go past the safety line towards home, force it back
        if (balloon.world_x > safety_line) {
            balloon.vx = -BALLOON_SPEED_X; // Go Left
        } 
        else {
            // Normal Homing
            if (balloon.world_x < target_x) balloon.vx = BALLOON_SPEED_X;
            else balloon.vx = -BALLOON_SPEED_X;
        }

        // --- Y Movement ---
        if (balloon.y < target_y) balloon.vy = BALLOON_SPEED_Y;
        else balloon.vy = -BALLOON_SPEED_Y;

        // --- Apply Physics ---
        balloon.world_x += balloon.vx;
        balloon.y += balloon.vy;

        // --- Clamp Altitude ---
        // If balloon is too low, force it up to the minimum altitude
        if (balloon.y > min_altitude) {
            balloon.y = min_altitude;
        }
    }

    // =========================================================
    // 3. COLLISION: PLAYER (The Balloon hits the Chopper)
    // =========================================================
    if (player_state == PLAYER_ALIVE && !balloon.is_falling) {

        int32_t target_x = chopper_world_x + (16 << SUBPIXEL_BITS);
        int32_t target_y = chopper_y + (8 << SUBPIXEL_BITS);
        
        // Calculate Balloon Center X (World X + 8px)
        int32_t balloon_cx = balloon.world_x + (8 << SUBPIXEL_BITS);
        
        // Balloon Center Y is just balloon.y 
        // (Based on render logic: Top is y-16, Bottom is y. Range is y-16 to y+16. Center is y.)
        int32_t balloon_cy = balloon.y; 

        // 3. Horizontal Check (Center to Center)
        // Combined radii is ~24px. We use 18px for a tight fit.
        if (labs(balloon_cx - target_x) < (18 << SUBPIXEL_BITS)) { 
            
            // 4. Vertical Check
            // Combined radii is ~24px. We use 18px.
            if (labs(balloon_cy - target_y) < (18 << SUBPIXEL_BITS)) {   
                
                // CRASH PLAYER
                kill_player();
                
                // DESTROY BALLOON
                trigger_explosion(balloon.world_x, balloon.y); 
                balloon.active = false;
                balloon.respawn_timer = BALLOON_RESPAWN;
                return;
            }
        }
    }

    // =========================================================
    // 4. RENDER
    // =========================================================
    
    // Animation (Slow bobbing)
    balloon.anim_timer++;
    if (balloon.anim_timer > 8) {
        balloon.anim_timer = 0;
        balloon.anim_frame++;
        if (balloon.anim_frame > 2) balloon.anim_frame = 0;
    }

    int32_t screen_sub = balloon.world_x - camera_x;
    int16_t screen_px = screen_sub >> SUBPIXEL_BITS;
    int16_t screen_y = balloon.y >> SUBPIXEL_BITS;

    if (screen_px > -16 && screen_px < 336) {
        
        // Draw Bottom
        xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_px);
        xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, y_pos_px, screen_y);
        xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_balloon_ptr(balloon.anim_frame, 0));

        // Draw Top (16px higher)
        xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_px);
        xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, y_pos_px, (screen_y - 16));
        xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_balloon_ptr(balloon.anim_frame, 1));
        
    } else {
        // Offscreen hide
        xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    }
}