#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "constants.h"
#include "player.h"
#include "hostages.h"
#include "enemybase.h"
#include "homebase.h"
#include "smallexplosion.h"


Hostage hostages[NUM_HOSTAGES];

// Gameplay Counters
uint8_t hostages_on_board = 0;
uint8_t hostages_rescued_count = 0;
uint8_t dropoff_timer = 0;

// Game stats
uint8_t hostages_lost_count = 0;

// Constants
#define SAFE_DOOR_DIST      (6 << SUBPIXEL_BITS)  // +/- 6 pixels from center is safe
#define CHOPPER_WIDTH_SUB   (32 << SUBPIXEL_BITS) 
#define CRUSH_WIDTH_SUB     (28 << SUBPIXEL_BITS) // Slightly narrower than full width

EnemyBase base_state[NUM_ENEMY_BASES]; // We track state for the 4 bases

// Helper to get sprite data offset
uint16_t get_hostage_ptr(int frame_idx) {
    return HOSTAGES_DATA + (frame_idx * 512);
}

void update_hostages(void) {

    // --- 1. PRE-CALCULATE CHOPPER STATE ---
    bool is_chopper_landed = (chopper_y >= GROUND_Y_SUB);
    
    // "Low Altitude" means the chopper body overlaps the hostage height
    // Chopper is ~16px high. If it's within 12px of the ground, it's dangerous.
    bool is_chopper_low = (chopper_y > (GROUND_Y_SUB - (12 << SUBPIXEL_BITS)));

    int32_t chopper_center_x = chopper_world_x + (16 << SUBPIXEL_BITS);
    
    // =========================================================
    // 1. SPAWN LOGIC (Enemy Bases)
    // =========================================================
    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        // Only spawn if base destroyed AND has people left
        if (base_state[i].destroyed && base_state[i].hostages_remaining > 0) {
            base_state[i].spawn_timer++;
            
            // Spawn delay (60 ticks)
            if (base_state[i].spawn_timer > 60) {
                
                // Try to find an OPEN slot in the sprite array
                for (int h = 0; h < NUM_HOSTAGES; h++) {
                    if (hostages[h].state == H_STATE_INACTIVE) {
                        // Found one! Spawn it.
                        base_state[i].spawn_timer = 0;
                        hostages[h].state = H_STATE_RUNNING_CHOPPER;
                        hostages[h].base_id = i;
                        hostages[h].world_x = ENEMY_BASE_LOCATIONS[i] + (13 << SUBPIXEL_BITS);
                        hostages[h].y = GROUND_Y_SUB + (4 << SUBPIXEL_BITS);
                        hostages[h].anim_frame = 8;
                        hostages[h].direction = 0;
                        
                        // Decrement the "World Population" count
                        base_state[i].hostages_remaining--;
                        break; // Stop looking for slots
                    }
                }
                // If no slots found (array full), we just try again next frame.
                // The spawn_timer keeps ticking, so they will spawn ASAP.
            }
        }
    }

    // =========================================================
    // 2. UNLOAD LOGIC (At Home Base)
    // =========================================================
    bool at_home_base = false;
    
    // Check if Landed
    if (chopper_y >= GROUND_Y_SUB) {
        
        // Define the narrow landing zone (3920 to 3945)
        // Use 'L' to ensure 32-bit math avoids overflow
        int32_t zone_min = 3920L << SUBPIXEL_BITS;
        int32_t zone_max = 3945L << SUBPIXEL_BITS;

        // Check if Chopper X is inside the zone
        if (chopper_world_x >= zone_min && chopper_world_x <= zone_max) {
            at_home_base = true;
        }
    }

    if (at_home_base && hostages_on_board > 0) {
        dropoff_timer++;
        if (dropoff_timer > 30) { 
            dropoff_timer = 0;
            // Find a passenger and eject them
            for (int h = 0; h < NUM_HOSTAGES; h++) {
                if (hostages[h].state == H_STATE_ON_BOARD) {
                    hostages[h].state = H_STATE_RUNNING_HOME;
                    hostages[h].world_x = chopper_world_x + (16 << SUBPIXEL_BITS);
                    // Mark the last one to Wave
                    hostages[h].base_id = (hostages_on_board == 1) ? 99 : 0;
                    hostages_on_board--;
                    break;
                }
            }
        }
    }

    // =========================================================
    // 3. HOSTAGE AI LOOP
    // =========================================================
    for (int i = 0; i < NUM_HOSTAGES; i++) {
        unsigned cfg = HOSTAGE_CONFIG + (i * sizeof(vga_mode4_sprite_t));

        // Skip inactive/hidden states to save CPU
        if (hostages[i].state == H_STATE_INACTIVE || 
            hostages[i].state == H_STATE_ON_BOARD || 
            hostages[i].state == H_STATE_SAFE) {
            continue; 
        }

        // Handle Dying (Recycle Slot immediately)
        if (hostages[i].state == H_STATE_DYING) {
            hostages_lost_count++; // Increment lost counter
            hostages[i].state = H_STATE_INACTIVE;
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
            continue;
        }

        // --- CRUSH CHECK (SQUISH) ---
        // Conditions:
        // 1. Hostage is vulnerable (Running/Waiting)
        // 2. Chopper is LOW (Touching ground or hovering slightly above)
        // 3. Chopper is NOT LANDED (Moving, taking off, or landing)
        // 4. Hostage is UNDER the chopper body (+/- 14px from center)
        if (is_chopper_low && !is_chopper_landed) {
            int32_t dist = labs(chopper_center_x - hostages[i].world_x);
            if (dist < (14 << SUBPIXEL_BITS)) {
                // SQUISH!
                hostages[i].state = H_STATE_DYING;
                
                // Visual Effect: Spawn small explosion at hostage position
                spawn_small_explosion(hostages[i].world_x + (8 << SUBPIXEL_BITS), hostages[i].y + (12 << SUBPIXEL_BITS));
                continue; // Stop processing this hostage
            }
        }

        int32_t target_x = hostages[i].world_x;
        bool move_logic = false;

        // --- RUNNING TO CHOPPER ---
        if (hostages[i].state == H_STATE_RUNNING_CHOPPER) {
            target_x = chopper_world_x + (16 << SUBPIXEL_BITS);
            move_logic = true;

            // Boarding Check
            if (chopper_y >= GROUND_Y_SUB) {
                int32_t diff_center = labs(target_x - hostages[i].world_x);
                if (diff_center <= SAFE_DOOR_DIST) {
                    hostages[i].state = H_STATE_ON_BOARD;
                    hostages_on_board++;
                    xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
                    continue;
                }
            }
        }
        // --- RUNNING HOME ---
        else if (hostages[i].state == H_STATE_RUNNING_HOME) {
            target_x = HOMEBASE_WORLD_X + (24 << SUBPIXEL_BITS);
            move_logic = true;

            // FIX: Increase threshold to 6 to ensure overlap with movement logic
            if (labs(target_x - hostages[i].world_x) < (6 << SUBPIXEL_BITS)) {
                
                if (hostages[i].base_id == 99) {
                    hostages[i].state = H_STATE_WAVING;
                    hostages[i].anim_timer = 0; 
                } else {
                    // Safe! Recycle Slot
                    hostages_rescued_count++;
                    hostages[i].state = H_STATE_INACTIVE; 
                    xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
                    continue; 
                }
            }
        }
        // --- WAVING ---
        else if (hostages[i].state == H_STATE_WAVING) {
            hostages[i].anim_timer++;
            if (hostages[i].anim_timer > 120) {
                // Done Waving! Recycle Slot
                hostages_rescued_count++;
                hostages[i].state = H_STATE_INACTIVE;
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
                continue;
            }
            move_logic = false;
        }

        // --- MOVEMENT PHYSICS ---
        int8_t intended_dir = 0;
        
        if (move_logic) {
            int32_t diff = target_x - hostages[i].world_x;
            if (labs(diff) > (4 << SUBPIXEL_BITS)) { 
                intended_dir = (diff > 0) ? 1 : -1;
            }

            // Spacing Logic
            if (intended_dir != 0) {
                for (int other = 0; other < NUM_HOSTAGES; other++) {
                    
                    // FIX: Ignore collision with hostages who are inside the chopper or safe base
                    if (i == other || 
                        hostages[other].state == H_STATE_INACTIVE || 
                        hostages[other].state == H_STATE_ON_BOARD || 
                        hostages[other].state == H_STATE_SAFE ||
                        hostages[other].state == H_STATE_DYING) {
                        continue;
                    }
                    
                    int32_t sep = hostages[other].world_x - hostages[i].world_x;
                    
                    // If we are moving Right, and someone is just ahead (< 12px)
                    if (intended_dir == 1 && sep > 0 && sep < (12<<4)) {
                        intended_dir = 0;
                    }
                    // If moving Left, and someone is just ahead (negative diff)
                    if (intended_dir == -1 && sep < 0 && sep > -(12<<4)) {
                        intended_dir = 0;
                    }
                }
            }

            if (intended_dir == 1)  hostages[i].world_x += HOSTAGE_RUN_SPEED;
            if (intended_dir == -1) hostages[i].world_x -= HOSTAGE_RUN_SPEED;
        }

        // --- ANIMATION UPDATE ---
        if (hostages[i].state != H_STATE_WAVING) {
            hostages[i].anim_timer++;
            if (hostages[i].anim_timer > 6) {
                hostages[i].anim_timer = 0;
                if (intended_dir == 1) {
                    hostages[i].anim_frame = (hostages[i].anim_frame < 2) ? hostages[i].anim_frame + 1 : 0;
                } else if (intended_dir == -1) {
                    hostages[i].anim_frame = (hostages[i].anim_frame < 3 || hostages[i].anim_frame > 4) ? 3 : hostages[i].anim_frame + 1;
                } else {
                    hostages[i].anim_frame = (hostages[i].anim_frame == 8) ? 9 : 8;
                }
            }
        } else {
            // Waving animation
            if ((hostages[i].anim_timer % 10) == 0) {
                hostages[i].anim_frame = (hostages[i].anim_frame == 8) ? 9 : 8;
            }
        }

        // --- RENDER ---
        int32_t screen_sub = hostages[i].world_x - camera_x;
        int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

        if (screen_px > -16 && screen_px < 336) {
            xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, screen_px);
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, hostages[i].y >> SUBPIXEL_BITS);
            xram0_struct_set(cfg, vga_mode4_sprite_t, xram_sprite_ptr, get_hostage_ptr(hostages[i].anim_frame));
        } else {
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
        }
    }
}