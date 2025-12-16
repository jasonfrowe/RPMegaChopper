#include <rp6502.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "constants.h"
#include "player.h"
#include "tanks.h"
#include "ebullets.h"
#include "smallexplosion.h"
#include "hostages.h"

// --- TANK AIMING TABLES ---
// Speed approx 4.5 pixels/frame (72 subpixels)
// Angles: 90 (Up), ~70, ~45, ~25, ~10 degrees

#define AIM_LEAD_Y      (32 << SUBPIXEL_BITS) // Aim 32px above chopper to compensate for gravity

#define VELOCITY_FACTOR 3
const int16_t TANK_AIM_VX[] = {
    0,   // Index 0: Up (0.0 px)
    11,  // Index 1: Steep (0.7 px)
    23,  // Index 2: Diag (1.4 px)
    29,  // Index 3: Shallow (1.8 px)
    32   // Index 4: Low (2.0 px)
};

const int16_t TANK_AIM_VY[] = {
    -32, // Index 0: Up (-2.0 px)
    -30, // Index 1: Steep (-1.8 px)
    -23, // Index 2: Diag (-1.4 px)
    -13, // Index 3: Shallow (-0.8 px)
    -5   // Index 4: Low (-0.3 px)
};

// --- TANK BULLET STATE ---
typedef struct {
    bool active;
    int32_t world_x;
    int32_t y;      // Using int32 for Y to handle the subpixel arc precision better
    int16_t vx;
    int16_t vy;
} TankBullet;

TankBullet tank_bullets[NEBULLET];

#define EBULLET_GROUND  (GROUND_Y_SUB + (14 << SUBPIXEL_BITS)) // Ground level for enemy bullets

void update_tank_bullets(void) {
    
    // =========================================================
    // 1. FIRING LOGIC (AIMED)
    // =========================================================
    for (int t = 0; t < NUM_TANKS; t++) {
        if (!tanks[t].active) continue;

        // Check Visibility
        int32_t screen_sub = tanks[t].world_x - camera_x;
        int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

        if (screen_px > -TANK_WIDTH_PX && screen_px < SCREEN_WIDTH + TANK_WIDTH_PX) {
            
            // Random Fire Chance (approx 1 per sec)
            if ((rand() % 100) < 2) { 
                
                for (int b = 0; b < NEBULLET; b++) {
                    if (!tank_bullets[b].active) {
                        
                        // --- SPAWN SETUP ---
                        tank_bullets[b].active = true;
                        
                        // Origin: Tank Turret (Center X, Top Y)
                        int32_t origin_x = tanks[t].world_x + (20 << SUBPIXEL_BITS);
                        int32_t origin_y = tanks[t].y - (2 << SUBPIXEL_BITS);
                        
                        tank_bullets[b].world_x = origin_x;
                        tank_bullets[b].y = origin_y;

                        // --- AIMING ALGORITHM ---
                        // Target: Center of Chopper, slightly above (Lead)
                        int32_t target_x = chopper_world_x + (16 << SUBPIXEL_BITS);
                        int32_t target_y = chopper_y - AIM_LEAD_Y;

                        // Calculate absolute distances
                        int32_t dx = labs(target_x - origin_x);
                        int32_t dy = origin_y - target_y; // Height diff (positive going up)

                        // Clamp dy to avoid divide-by-zero or weirdness if chopper is below tank
                        if (dy < (16 << SUBPIXEL_BITS)) dy = (16 << SUBPIXEL_BITS);

                        // Select Angle Index based on Slope (dx / dy)
                        // We use shifts to approximate ratios without division
                        // 0: dx is very small (< 0.25 dy)
                        // 1: dx is small (< 0.75 dy)
                        // 2: dx is medium (< 1.5 dy)
                        // 3: dx is large (< 3.0 dy)
                        // 4: dx is very large (Flat)
                        
                        int aim_idx = 0;
                        if (dx < (dy >> 2)) {
                            aim_idx = 0; // Straight Up
                        } 
                        else if (dx < (dy - (dy >> 2))) {
                            aim_idx = 1; // Steep
                        }
                        else if (dx < (dy + (dy >> 1))) {
                            aim_idx = 2; // Diagonal (45 deg)
                        }
                        else if (dx < ((dy << 1) + dy)) { // dy * 3
                            aim_idx = 3; // Shallow
                        }
                        else {
                            aim_idx = 4; // Low
                        }

                        // Set Velocity from Lookup Table
                        tank_bullets[b].vy =  VELOCITY_FACTOR * TANK_AIM_VY[aim_idx];
                        
                        // Set Horizontal Velocity (Flip based on direction)
                        if (target_x > origin_x) {
                            tank_bullets[b].vx =  VELOCITY_FACTOR * TANK_AIM_VX[aim_idx]; // Fire Right
                        } else {
                            tank_bullets[b].vx = -VELOCITY_FACTOR * TANK_AIM_VX[aim_idx]; // Fire Left
                        }

                        break; // Fired!
                    }
                }
            }
        }
    }

    // =========================================================
    // 2. PHYSICS & COLLISION
    // =========================================================
    for (int b = 0; b < NEBULLET; b++) {
        unsigned cfg = EBULLET_CONFIG + (b * sizeof(vga_mode4_sprite_t));

        if (!tank_bullets[b].active) {
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
            continue;
        }

        // --- MOVE ---
        tank_bullets[b].world_x += tank_bullets[b].vx;
        tank_bullets[b].y       += tank_bullets[b].vy;
        tank_bullets[b].vy      += TANK_BULLET_GRAVITY; // Gravity Apply

        // --- GROUND CHECK ---
        // Hit the "Main Ground" (Hostage layer), not the "Tank Ground"
        if (tank_bullets[b].y > EBULLET_GROUND && tank_bullets[b].vy  > 0){
            tank_bullets[b].active = false;
            // Optional: Spawn small explosion on ground
            spawn_small_explosion(tank_bullets[b].world_x, EBULLET_GROUND);
            continue;
        }

        // --- HOSTAGE COLLISION (Cruelty Check) ---
        // Requirement: Bullet must be FALLING (vy > 0)
        if (tank_bullets[b].vy > 0) {
            
            // Optimization: Only check if bullet is near ground level
            // Hostages are 16px tall. Check if bullet is in that band.
            if (tank_bullets[b].y > (EBULLET_GROUND - (20 << SUBPIXEL_BITS))) {
                
                for (int h = 0; h < NUM_HOSTAGES; h++) {
                    // Only check vulnerable hostages
                    if (hostages[h].state == H_STATE_RUNNING_CHOPPER || 
                        hostages[h].state == H_STATE_RUNNING_HOME ||
                        hostages[h].state == H_STATE_WAVING) {

                        // Hitbox check
                        int32_t host_cx = hostages[h].world_x + (8 << SUBPIXEL_BITS);
                        int32_t dist = labs(tank_bullets[b].world_x - host_cx);

                        if (dist < (6 << SUBPIXEL_BITS)) { // Direct hit
                            // Kill Hostage
                            int32_t host_cx = hostages[h].world_x + (8 << SUBPIXEL_BITS);
                            hostages[h].state = H_STATE_DYING;
                            spawn_small_explosion(host_cx, hostages[h].y + (8<<SUBPIXEL_BITS));
                            
                            // Destroy Bullet
                            tank_bullets[b].active = false;
                            break; 
                        }
                    }
                }
                if (!tank_bullets[b].active) continue; // Bullet died, next bullet
            }
        }

        // --- RENDER ---
        int32_t screen_sub = tank_bullets[b].world_x - camera_x;
        int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

        if (screen_px > -4 && screen_px < 324) {
            xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, screen_px);
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, tank_bullets[b].y >> SUBPIXEL_BITS);
        } else {
            // Off-screen = Deactivate to save slots
            tank_bullets[b].active = false;
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
        }
    }
}


