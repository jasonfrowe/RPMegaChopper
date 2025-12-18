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
#include <stdlib.h>
#include "balloon.h"
#include "jet.h"
#include "sound.h"
#include "boom.h"

// --- BULLET STATE ---
bool bullet_active = false;
int32_t bullet_world_x = 0;
int16_t bullet_y = 0;
int16_t bullet_vx = 0;
int16_t bullet_vy = 0;

// Input tracking for semi-automatic fire
int player_fire_cooldown = 0;

// Configuration
#define BULLET_SPEED_X    (8 << SUBPIXEL_BITS) // Fast horizontal speed
#define BULLET_SPEED_Y    (2 << SUBPIXEL_BITS) // Slight vertical angle
#define BULLET_Y_OFFSET   (12 << SUBPIXEL_BITS) // Offset from chopper center
#define BULLET_GROUND     (GROUND_Y_SUB + (12 << SUBPIXEL_BITS)) // Ground level for bullets

void update_bullet(void) {


    if (player_fire_cooldown > 0) {
        player_fire_cooldown--;
    }


    bool fire_pressed = is_action_pressed(0, ACTION_FIRE);

    // -----------------------------------------------------------
    // 1. SPAWN LOGIC
    // -----------------------------------------------------------
    if (fire_pressed && player_fire_cooldown == 0 && !bullet_active && !is_landed) {
        
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
            player_fire_cooldown = 20; // 20 frame cooldown
            // play_sound(SFX_TYPE_PLAYER_FIRE, 110, PSG_WAVE_SQUARE, 0, 3, 4, 2);
            sfx_player_shoot();
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

    // -----------------------------------------------------------
    // 4. CHECK JET
    // -----------------------------------------------------------
    // External check for jet.state
    if (jet.state != JET_INACTIVE) {
        // Jet is 16px wide, 8px tall.
        // Center: WorldX + 8, Y + 4.
        int32_t jet_cx = jet.world_x + (8 << SUBPIXEL_BITS);
        int32_t jet_cy = jet.y + (4 << SUBPIXEL_BITS);

        if (labs(bullet_world_x - jet_cx) < (12 << SUBPIXEL_BITS)) {
            if (labs(bullet_y - jet_cy) < (8 << SUBPIXEL_BITS)) {
                
                // HIT!
                jet.state = JET_INACTIVE;
                jet.weapon_active = false; // Kill weapon too? Or let it fall? Usually kill it for fairness.
                
                trigger_explosion(jet.world_x, jet.y);
                sfx_explosion_small();
                
                bullet_active = false;
                xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
                
                // Hide sprites immediately
                xram0_struct_set(JET_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
                xram0_struct_set(JET_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
                
                return;
            }
        }
    }

    // -----------------------------------------------------------
    // 3. CHECK BALLOON
    // -----------------------------------------------------------
    // Only hit if active and NOT already falling
    if (balloon.active && !balloon.is_falling) {
        
        // 1. Calculate Centers
        int32_t balloon_cx = balloon.world_x + (8 << SUBPIXEL_BITS);

        // Balloon Y is Bottom Edge. Height is 32px. Center is +16px (Down).
        int32_t balloon_cy = balloon.y + (0 << SUBPIXEL_BITS);

        // 2. Horizontal Check (10px reach)
        if (labs(bullet_world_x - balloon_cx) < (10 << SUBPIXEL_BITS)) {
            
            // 3. Vertical Check
            // Height is 32px. Half-height is 16px.
            // Using 16px threshold covers the full height exactly with bullet center.
            if (labs(bullet_y - balloon_cy) < (18 << SUBPIXEL_BITS)) {
                
                // --- HIT! ---
                // Disable Bullet
                bullet_active = false;
                xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);

                // --- TRIGGER FALL ---
                balloon.is_falling = true;

                // --- TRIGGER VISUAL BOOM ---
                // Calculate screen coords for the boom
                // Center it on the balloon (Balloon Y is the split point)
                // Boom is 16px tall. To center on Y, Top-Left should be Y - 8.
                int16_t boom_scr_x = (balloon.world_x - camera_x) >> SUBPIXEL_BITS;
                int16_t boom_scr_y = (balloon.y >> SUBPIXEL_BITS) - 8; 
                
                // Physics: Stop horizontal movement, start dropping
                balloon.vx = 0; 
                balloon.vy = (1 << SUBPIXEL_BITS); // Initial downward nudge

                // Trigger explosion at balloon location
                trigger_boom(boom_scr_x, boom_scr_y);
                
                // Optional: Play a small "thud" sound to indicate damage
                sfx_explosion_small();
                
                return;
            }
        }
    }

    // -----------------------------------------------------------
    // CHECK ENEMY BASES
    // -----------------------------------------------------------
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
                sfx_explosion_small();

                // Hide bullet sprite immediately
                xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
                
                return; // Only hit one thing at a time
            }
        }
    }

    // -----------------------------------------------------------
    // CHECK HOSTAGES (Friendly Fire)
    // -----------------------------------------------------------
    // Hitbox: +/- 5 pixels width, +/- 7 pixels height
    const int32_t MAN_HALF_W = (5 << SUBPIXEL_BITS);
    const int32_t MAN_HALF_H = (7 << SUBPIXEL_BITS);

    for (int i = 0; i < NUM_HOSTAGES; i++) {
        // Only check vulnerable hostages
        if (hostages[i].state == H_STATE_RUNNING_CHOPPER || 
            hostages[i].state == H_STATE_RUNNING_HOME || 
            hostages[i].state == H_STATE_WAVING) {

            // Calculate Hostage Center
            // World X is top-left. Center is +8.
            int32_t h_center_x = hostages[i].world_x + (8 << SUBPIXEL_BITS);
            int32_t h_center_y = hostages[i].y + (8 << SUBPIXEL_BITS); 

            // Check X Distance
            if (labs(bullet_world_x - h_center_x) < MAN_HALF_W) {
                // Check Y Distance
                if (labs(bullet_y - h_center_y) < MAN_HALF_H) {
                    
                    // --- HOSTAGE HIT! ---
                    hostages[i].state = H_STATE_DYING;
                    
                    // Visuals
                    spawn_small_explosion(hostages[i].world_x + (8 << SUBPIXEL_BITS), hostages[i].y + (12 << SUBPIXEL_BITS));
                    sfx_hostage_die();
                    
                    // Destroy Bullet
                    bullet_active = false;
                    xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
                    return;
                }
            }
        }
    }
}
