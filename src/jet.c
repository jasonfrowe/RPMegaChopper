#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> // for rand()
#include "constants.h"
#include "player.h"
#include "input.h" // camera_x
#include "hostages.h" // hostages_total_spawned
#include "jet.h"
#include "smallexplosion.h"
#include "explosion.h"
#include "sound.h"


#define JET_GROUND_Y_SUB  (GROUND_Y_SUB + (14 << SUBPIXEL_BITS)) // Ground level for enemy bullets

FighterJet jet;

// Global Timers (Reset when conditions fail)
uint16_t timer_loiter_ground = 0;
uint16_t timer_loiter_air = 0;

// Sprite Helper
// 8x8 sprite = 128 bytes
uint16_t get_jet_ptr(int index) {
    return JET_DATA + (index * 128);
}

void reset_jet(void) {
    // Reset Logic State
    jet.state = JET_INACTIVE;
    jet.weapon_active = false;
    
    // Reset Timers so it doesn't spawn instantly upon respawn
    timer_loiter_ground = 0;
    timer_loiter_air = 0;

    // Hide Sprites (Hardware)
    xram0_struct_set(JET_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    xram0_struct_set(JET_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    xram0_struct_set(JET_BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    xram0_struct_set(JET_BOMB_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
}

void update_jet(void) {
    int total_progress = hostages_total_spawned;
    
    // =========================================================
    // 1. SPAWN TIMERS & TRIGGERS
    // =========================================================
    if (jet.state == JET_INACTIVE) {
        
        // Progress Gate
        if (total_progress < JET_MIN_PROGRESS) return;

        // Safety Line Check (Don't spawn near Home)
        // Midpoint between Base 3 and Home
        // Assuming ENEMY_BASE_LOCATIONS is visible or defined
        // We can just use a hardcoded value if needed, or extern the array
        int32_t safety_line = (3500L + 3900L) / 2; // Approx 3700 world pixels
        safety_line = safety_line << SUBPIXEL_BITS;

        if (chopper_world_x > safety_line) {
            // Too close to home, reset timers
            timer_loiter_ground = 0;
            timer_loiter_air = 0;
            return;
        }

        bool try_spawn = false;
        JetWeaponType mode = WEAPON_NONE;

        // A. Ground Loiter Check
        if (chopper_y >= GROUND_Y_SUB) {
            timer_loiter_ground++;
            if (timer_loiter_ground > TIMER_GROUND_MAX) {
                if ((rand() % 100) < 2) { // Random chance per frame once threshold met
                    try_spawn = true;
                    mode = WEAPON_BOMB;
                }
            }
        } else {
            timer_loiter_ground = 0;
        }

        // B. Air Loiter Check
        if (chopper_y < AIR_ZONE_Y) {
            timer_loiter_air++;
            if (timer_loiter_air > TIMER_AIR_MAX) {
                if ((rand() % 100) < 2) {
                    try_spawn = true;
                    mode = WEAPON_BULLET;
                }
            }
        } else {
            timer_loiter_air = 0;
        }

        // --- SPAWN EXECUTION ---
        if (try_spawn) {
            jet.state = JET_FLYING_ATTACK;
            jet.has_fired = false;
            jet.weapon_type = mode;
            jet.weapon_active = false;
            
            // Random Direction
            jet.direction = (rand() % 2 == 0) ? 1 : -1;

            // X Position: Offscreen
            if (jet.direction == 1) { // Moving Right (Spawn Left)
                jet.world_x = camera_x - (32 << SUBPIXEL_BITS);
            } else { // Moving Left (Spawn Right)
                jet.world_x = camera_x + (320 << SUBPIXEL_BITS) + (32 << SUBPIXEL_BITS);
            }

            // Y Position
            if (mode == WEAPON_BOMB) {
                // Bomber flies fairly low to hit ground target accurately
                jet.y = GROUND_Y_SUB - (80 << SUBPIXEL_BITS); 
            } else {
                // Interceptor flies at player altitude
                jet.y = chopper_y + (12 << SUBPIXEL_BITS);
            }
            
            // Reset timers
            timer_loiter_ground = 0;
            timer_loiter_air = 0;
        }
        
        // Hide Sprites if inactive
        xram0_struct_set(JET_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(JET_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(JET_BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(JET_BOMB_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        return;
    }

    // =========================================================
    // 2. JET MOVEMENT AI
    // =========================================================
    
    // Horizontal Move
    if (jet.direction == 1) jet.world_x += JET_SPEED_X;
    else jet.world_x -= JET_SPEED_X;

    // ATTACK LOGIC
    if (!jet.has_fired && jet.state == JET_FLYING_ATTACK) {
        
        // Horizontal distance to chopper center
        int32_t chop_cx = chopper_world_x + (16 << SUBPIXEL_BITS);
        int32_t dist = jet.world_x - chop_cx; // Signed

        if (jet.weapon_type == WEAPON_BOMB) {
            
            // Physics: Bomb travels forward ~65px while falling from 80px height
            const int32_t DROP_LEAD = (58 << SUBPIXEL_BITS); //72 was the expected value
            const int32_t DROP_WINDOW = (10 << SUBPIXEL_BITS); // Tolerance
            
            bool should_drop = false;

            if (jet.direction == 1) {
                // Moving RIGHT. Approaching from Left.
                // We want to drop when we are LEFT of the chopper (-70px relative)
                // dist (Jet - Chop) should be approx -70.
                if (dist > -(DROP_LEAD + DROP_WINDOW) && dist < -(DROP_LEAD - DROP_WINDOW)) {
                    should_drop = true;
                }
            } 
            else {
                // Moving LEFT. Approaching from Right.
                // We want to drop when we are RIGHT of the chopper (+70px relative)
                // dist (Jet - Chop) should be approx +70.
                if (dist > (DROP_LEAD - DROP_WINDOW) && dist < (DROP_LEAD + DROP_WINDOW)) {
                    should_drop = true;
                }
            }

            if (should_drop) {
                jet.weapon_active = true;
                jet.w_x = jet.world_x + (8 << SUBPIXEL_BITS); // Center of jet
                jet.w_y = jet.y + (8 << SUBPIXEL_BITS);
                
                // Inherit momentum (2.0 pixels/frame)
                jet.w_vx = (jet.direction == 1) ? (2 << 4) : -(2 << 4); 
                jet.w_vy = 0; 
                
                jet.has_fired = true;
                jet.state = JET_ARCING_AWAY; // Fly away
                sfx_bomb_drop();
            }
        }
        // BULLET: Fire when in range but not too close
        else if (jet.weapon_type == WEAPON_BULLET) {
            // Fire at 100 pixels distance
            if (labs(dist) < (100 << SUBPIXEL_BITS)) {
                jet.weapon_active = true;
                jet.w_x = jet.world_x + ((jet.direction == 1 ? 16 : 0) << SUBPIXEL_BITS);
                jet.w_y = jet.y + (4 << SUBPIXEL_BITS);
                
                // Fire towards chopper
                jet.w_vx = (jet.direction == 1) ? JET_BULLET_SPEED : -JET_BULLET_SPEED;
                jet.w_vy = 0; // Straight shot? or aimed? Straight is harder to dodge at speed.
                
                jet.has_fired = true;
                jet.state = JET_ARCING_AWAY;
                sfx_enemy_shoot();
            }
        }
    }

    // ARC AWAY
    if (jet.state == JET_ARCING_AWAY) {
        // Climb rapidly to exit screen
        jet.y -= JET_CLIMB_SPEED;
    }

    // DESPAWN CHECK
    int32_t screen_sub = jet.world_x - camera_x;
    int16_t screen_px = screen_sub >> SUBPIXEL_BITS;
    int16_t screen_y = jet.y >> SUBPIXEL_BITS;

    if (screen_px < -32 || screen_px > 350 || screen_y < -32) {
        jet.state = JET_INACTIVE;
    }

    // =========================================================
    // 3. WEAPON PHYSICS
    // =========================================================
    if (jet.weapon_active) {
        jet.w_x += jet.w_vx;
        jet.w_y += jet.w_vy;

        if (jet.weapon_type == WEAPON_BOMB) {
            jet.w_vy += JET_BOMB_GRAVITY; // Gravity
            
            // Ground Check
            if (jet.w_y >= JET_GROUND_Y_SUB) {
                jet.weapon_active = false;
                spawn_small_explosion(jet.w_x, JET_GROUND_Y_SUB);
                // Check if it hit the chopper on ground?
                // For fairness, maybe bombs only kill chopper if direct hit, 
                // but let's assume direct hit logic below covers it.
            }
        }

        // OFFSCREEN CHECK
        int32_t w_screen_sub = jet.w_x - camera_x;
        int16_t w_px = w_screen_sub >> SUBPIXEL_BITS;
        if (w_px < -10 || w_px > 330) jet.weapon_active = false;
    }

    // =========================================================
    // 4. COLLISION: WEAPON VS PLAYER
    // =========================================================
    if (jet.weapon_active && player_state == PLAYER_ALIVE) {
        int32_t chop_cx = chopper_world_x + (16 << SUBPIXEL_BITS);
        int32_t chop_cy = chopper_y + (8 << SUBPIXEL_BITS);
        
        // Hitbox: +/- 10px
        if (labs(jet.w_x - chop_cx) < (10 << SUBPIXEL_BITS)) {
            if (labs(jet.w_y - chop_cy) < (10 << SUBPIXEL_BITS)) {
                kill_player();
                trigger_explosion(jet.w_x, jet.w_y);
                jet.weapon_active = false;
            }
        }
    }

    // =========================================================
    // 5. RENDER
    // =========================================================
    if (jet.state != JET_INACTIVE) {
        // Draw Jet (Two 8x8 sprites side-by-side)
        // Direction 1 (Right): Indices 2, 3
        // Direction -1 (Left): Indices 0, 1
        int idx1 = (jet.direction == 1) ? 2 : 0;
        int idx2 = idx1 + 1;

        xram0_struct_set(JET_LEFT_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_px);
        xram0_struct_set(JET_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, screen_y);
        xram0_struct_set(JET_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_jet_ptr(idx1));

        xram0_struct_set(JET_RIGHT_CONFIG, vga_mode4_sprite_t, x_pos_px, (screen_px + 8));
        xram0_struct_set(JET_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, screen_y);
        xram0_struct_set(JET_RIGHT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_jet_ptr(idx2));
    }

    // Draw Weapon
    if (jet.weapon_active) {
        unsigned w_cfg = (jet.weapon_type == WEAPON_BOMB) ? JET_BOMB_CONFIG : JET_BULLET_CONFIG;
        unsigned other = (jet.weapon_type == WEAPON_BOMB) ? JET_BULLET_CONFIG : JET_BOMB_CONFIG;
        
        int16_t w_px = (jet.w_x - camera_x) >> SUBPIXEL_BITS;
        xram0_struct_set(w_cfg, vga_mode4_sprite_t, x_pos_px, w_px);
        xram0_struct_set(w_cfg, vga_mode4_sprite_t, y_pos_px, jet.w_y >> SUBPIXEL_BITS);
        
        // Hide the unused weapon config
        xram0_struct_set(other, vga_mode4_sprite_t, y_pos_px, -32);
    } else {
        xram0_struct_set(JET_BOMB_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(JET_BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    }
}

