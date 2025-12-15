#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "constants.h"
#include "player.h"
#include "hostages.h"
#include "enemybase.h"


Hostage hostages[NUM_HOSTAGES];



EnemyBase base_state[NUM_ENEMY_BASES]; // We track state for the 4 bases

// Helper to get sprite data offset
uint16_t get_hostage_ptr(int frame_idx) {
    return HOSTAGES_DATA + (frame_idx * 512);
}

void update_hostages(void) {
    
    // ---------------------------------------------------
    // 1. SPAWN LOGIC
    // ---------------------------------------------------
    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        if (base_state[i].destroyed && base_state[i].hostages_remaining > 0) {
            
            base_state[i].spawn_timer++;
            
            // Spawn one every 60 ticks (1 second)
            if (base_state[i].spawn_timer > 60) {
                base_state[i].spawn_timer = 0;
                
                // Find a free slot
                for (int h = 0; h < NUM_HOSTAGES; h++) {
                    if (!hostages[h].active) {
                        hostages[h].active = true;
                        hostages[h].base_id = i;
                        hostages[h].world_x = ENEMY_BASE_LOCATIONS[i]; // Spawn at base
                        hostages[h].y = GROUND_Y_SUB + (4 << SUBPIXEL_BITS); // On ground
                        hostages[h].anim_frame = 8; // Idle
                        hostages[h].direction = 0;
                        
                        base_state[i].hostages_remaining--;
                        break;
                    }
                }
            }
        }
    }

    // ---------------------------------------------------
    // 2. AI & MOVEMENT
    // ---------------------------------------------------
    for (int i = 0; i < NUM_HOSTAGES; i++) {
        if (!hostages[i].active) continue;

        // A. Determine Goal Direction (Towards Chopper)
        int32_t target_x = chopper_world_x + (8 << SUBPIXEL_BITS); // Aim for center of chopper
        int32_t diff = target_x - hostages[i].world_x;
        int32_t dist = labs(diff);
        int8_t intended_dir = 0;

        // If chopper is close (within 10 pixels), stop
        if (dist < (10 << SUBPIXEL_BITS)) {
            intended_dir = 0;
        } else if (diff > 0) {
            intended_dir = 1; // Right
        } else {
            intended_dir = -1; // Left
        }

        // B. Overlap Check (Spacing)
        // Check against all other active hostages
        bool blocked = false;
        if (intended_dir != 0) {
            for (int other = 0; other < NUM_HOSTAGES; other++) {
                if (i == other || !hostages[other].active) continue;

                int32_t separation = hostages[other].world_x - hostages[i].world_x;
                
                // If we are moving Right, and someone is just ahead of us (< 16px)
                if (intended_dir == 1 && separation > 0 && separation < (12 << 4)) {
                    blocked = true; break;
                }
                // If moving Left, and someone is just ahead (negative diff)
                if (intended_dir == -1 && separation < 0 && separation > -(12 << 4)) {
                    blocked = true; break;
                }
            }
        }

        if (blocked) intended_dir = 0; // Wait your turn!

        // C. Apply Movement
        hostages[i].direction = intended_dir;
        if (intended_dir == 1)  hostages[i].world_x += HOSTAGE_RUN_SPEED;
        if (intended_dir == -1) hostages[i].world_x -= HOSTAGE_RUN_SPEED;

        // ---------------------------------------------------
        // 3. ANIMATION
        // ---------------------------------------------------
        hostages[i].anim_timer++;
        if (hostages[i].anim_timer > 6) { // Animate every 6 ticks
            hostages[i].anim_timer = 0;
            
            if (intended_dir == 1) {
                // Running Right: Cycle 0 -> 1 -> 2
                if (hostages[i].anim_frame < 2) hostages[i].anim_frame++;
                else hostages[i].anim_frame = 0;
            } 
            else if (intended_dir == -1) {
                // Running Left: Cycle 3 -> 4 -> 5
                if (hostages[i].anim_frame < 3 || hostages[i].anim_frame > 4) hostages[i].anim_frame = 3;
                else hostages[i].anim_frame++;
            }
            else {
                // Idle: Cycle 8 -> 9
                if (hostages[i].anim_frame == 8) hostages[i].anim_frame = 9;
                else hostages[i].anim_frame = 8;
            }
        }

        // ---------------------------------------------------
        // 4. HARDWARE UPDATE
        // ---------------------------------------------------
        unsigned cfg = HOSTAGE_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        
        // Calculate Screen Position
        int32_t screen_sub = hostages[i].world_x - camera_x;
        int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

        // Culling
        if (screen_px > -16 && screen_px < 336) {
            xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, screen_px);
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, hostages[i].y >> SUBPIXEL_BITS);
            xram0_struct_set(cfg, vga_mode4_sprite_t, xram_sprite_ptr, get_hostage_ptr(hostages[i].anim_frame));
        } else {
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
        }
    }
}