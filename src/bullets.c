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
#define BULLET_GROUND     (GROUND_Y_SUB + (16 << SUBPIXEL_BITS)) // Ground level for bullets

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

    // Base Width is 64 pixels (Two 32x32 sprites side-by-side)
    // Hitbox: +/- 32 pixels from center
    const int32_t HITBOX_HALF_WIDTH = (32 << SUBPIXEL_BITS);

    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        // Skip if already destroyed
        if (base_state[i].destroyed) continue;

        int32_t base_x = ENEMY_BASE_LOCATIONS[i];
        
        // Simple 1D collision check (World X)
        // Check if bullet is within the base width
        if (bullet_world_x > (base_x - HITBOX_HALF_WIDTH) && 
            bullet_world_x < (base_x + HITBOX_HALF_WIDTH)) {
            
            // Check Y height (Base is ~32px tall)
            // Bullet Y is screen relative in your code, but stored as absolute Y? 
            // Assuming bullet_y is World Y (pixels from top of screen):
            if (bullet_y > (GROUND_Y_SUB - (32<<4))) {
                
                // --- HIT! ---
                base_state[i].destroyed = true;
                bullet_active = false; // Destroy bullet

                // TRIGGER EXPLOSION CENTERED ON BASE
                // Base is 64px wide. Center is world_x + 32.
                // Explosion is 32px wide. To center it: (BaseX + 32) - (ExpWidth/2) = BaseX + 16
                trigger_explosion(base_x + (16 << SUBPIXEL_BITS), GROUND_Y_SUB - (4 << SUBPIXEL_BITS));
                
                // Hide bullet sprite immediately
                xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
                
                return; // Only hit one thing at a time
            }
        }
    }
}
