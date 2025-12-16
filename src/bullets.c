#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include "constants.h"
#include "player.h"
#include "bullets.h"
#include "enemybase.h"
#include "input.h"
#include "hostages.h"
#include "explosion.h"
#include "smallexplosion.h"

// --- BULLET STATE ---
bool bullet_active = false;
int32_t bullet_world_x = 0;
int16_t bullet_y = 0;
int16_t bullet_vx = 0;
int16_t bullet_vy = 0;

// Input tracking for semi-automatic fire
bool fire_cooldown = 0;

// Configuration
#define BULLET_SPEED_X    (8 << SUBPIXEL_BITS) // Fast horizontal speed
#define BULLET_SPEED_Y    (2 << SUBPIXEL_BITS) // Slight vertical angle
#define BULLET_Y_OFFSET   (12 << SUBPIXEL_BITS) // Offset from chopper center
#define BULLET_GROUND     (GROUND_Y_SUB + (12 << SUBPIXEL_BITS)) // Ground level for bullets

void update_bullet(void) {
    bool fire_pressed = is_action_pressed(0, ACTION_FIRE);

    // -----------------------------------------------------------
    // 1. SPAWN LOGIC
    // -----------------------------------------------------------
    if (fire_pressed && !fire_cooldown && !bullet_active && !is_landed) {
        
        bool can_fire = false;

        // --- FACING RIGHT ---
        if (current_heading == FACING_RIGHT) {
            can_fire = true;
            bullet_world_x = chopper_world_x + (24 << SUBPIXEL_BITS); // Nose
            bullet_vx = BULLET_SPEED_X;

            // Angle Logic based on Animation Frame
            if (base_frame == FRAME_RIGHT_ACCEL) {
                // Nose Down -> Fire Down
                bullet_vy = BULLET_SPEED_Y; 
            } 
            else if (base_frame == FRAME_RIGHT_BRAKE) {
                // Nose Up -> Fire Up
                bullet_vy = -BULLET_SPEED_Y;
            } 
            else {
                // Idle -> Fire Straight
                bullet_vy = 0;
            }
        }
        // --- FACING LEFT ---
        else if (current_heading == FACING_LEFT) {
            can_fire = true;
            bullet_world_x = chopper_world_x + (0 << SUBPIXEL_BITS); // Nose
            bullet_vx = -BULLET_SPEED_X;

            // Angle Logic
            if (base_frame == FRAME_LEFT_ACCEL) {
                // Nose Down -> Fire Down
                bullet_vy = BULLET_SPEED_Y;
            } 
            else if (base_frame == FRAME_LEFT_BRAKE) {
                // Nose Up -> Fire Up
                bullet_vy = -BULLET_SPEED_Y;
            } 
            else {
                bullet_vy = 0;
            }
        }

        if (can_fire) {
            bullet_active = true;
            bullet_y = chopper_y + BULLET_Y_OFFSET;
        }
    }

    // -----------------------------------------------------------
    // 2. PHYSICS & CULLING
    // -----------------------------------------------------------
    if (bullet_active) {
        bullet_world_x += bullet_vx;
        bullet_y += bullet_vy;

        // Ground Check
        if (bullet_y > BULLET_GROUND) {
            bullet_active = false;

            // Trigger Explosion at bullet location, clamped to ground level
            spawn_small_explosion(bullet_world_x, BULLET_GROUND + (4 << SUBPIXEL_BITS));
        }

        // Screen Boundary Check
        // We calculate screen pos to see if it left the camera view
        int32_t screen_sub = bullet_world_x - camera_x;
        int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

        if (screen_px < -16 || screen_px > 336) {
            bullet_active = false;
        }
    }

    // -----------------------------------------------------------
    // 3. HARDWARE UPDATE
    // -----------------------------------------------------------
    if (bullet_active) {
        int16_t screen_px = (bullet_world_x - camera_x) >> SUBPIXEL_BITS;
        
        xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_px);
        xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, bullet_y >> SUBPIXEL_BITS);
    } else {
        // Hide
        xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    }
}

void check_bullet_collisions(void) {
    if (!bullet_active) return;

    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        // Skip if already destroyed
        if (base_state[i].destroyed) continue;

        int32_t base_left_x = ENEMY_BASE_LOCATIONS[i];

        // ---------------------------------------------------
        // 1. HORIZONTAL CHECK (20-25 pixels from Left)
        // ---------------------------------------------------
        int32_t hit_min_x = base_left_x + (18 << SUBPIXEL_BITS);
        int32_t hit_max_x = base_left_x + (27 << SUBPIXEL_BITS);

        if (bullet_world_x >= hit_min_x && bullet_world_x <= hit_max_x) {
            
            // ---------------------------------------------------
            // 2. VERTICAL CHECK (4-9 pixels from Bottom)
            // ---------------------------------------------------
            // Y grows DOWN. 
            // "Bottom" is GROUND_Y_SUB.
            // 4 pixels up = Ground - 4
            // 9 pixels up = Ground - 9
            
            int16_t hit_bottom_y = GROUND_Y_SUB + (13 << SUBPIXEL_BITS);
            int16_t hit_top_y    = GROUND_Y_SUB + (4 << SUBPIXEL_BITS);

            // Check if bullet is within this vertical slice
            if (bullet_y >= hit_top_y && bullet_y <= hit_bottom_y) {
                
                // --- HIT! ---
                base_state[i].destroyed = true;
                bullet_active = false; 
                
                // Trigger explosion
                // Centered on the base (Base X + 16px to center the 32px explosion on the 64px building)
                trigger_explosion(base_left_x + (16 << SUBPIXEL_BITS), GROUND_Y_SUB - (4 << SUBPIXEL_BITS));

                // Hide bullet sprite immediately
                xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
                
                return; // Only hit one thing at a time
            }
        }
    }
}
