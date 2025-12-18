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
#include "sound.h"
#include "input.h"


Hostage hostages[NUM_HOSTAGES];

// Gameplay Counters
uint8_t hostages_on_board = 0;
uint8_t hostages_rescued_count = 0;
uint8_t dropoff_timer = 0;
uint8_t hostages_total_spawned = 0;

// Game stats
uint8_t hostages_lost_count = 0;

// Constants
#define SAFE_DOOR_DIST      (6 << SUBPIXEL_BITS)  // +/- 6 pixels from center is safe
#define CHOPPER_WIDTH_SUB   (32 << SUBPIXEL_BITS) 
#define CRUSH_WIDTH_SUB     (28 << SUBPIXEL_BITS) // Slightly narrower than full width

#define SIGHT_HEIGHT ((GROUND_Y_SUB - (64<<SUBPIXEL_BITS)))

EnemyBase base_state[NUM_ENEMY_BASES]; // We track state for the 4 bases

// Helper to get sprite data offset
uint16_t get_hostage_ptr(int frame_idx) {
    return HOSTAGES_DATA + (frame_idx * 512);
}

void kill_all_passengers(void) {
    for (int i = 0; i < NUM_HOSTAGES; i++) {
        // If they are on board (or mid-boarding), they die
        if (hostages[i].state == H_STATE_ON_BOARD || 
            hostages[i].state == H_STATE_BOARDING) {
            
            hostages[i].state = H_STATE_INACTIVE;
            
            // Note: We don't spawn explosions here because they are inside 
            // the chopper, which is already exploding.
        }
    }
    
    // Update Global Counts
    hostages_lost_count += hostages_on_board;
    hostages_on_board = 0;
}

void update_hostages(void) {
    
    // --- 1. PRE-CALCULATE CHOPPER STATE ---
    bool is_chopper_landed = (chopper_y >= GROUND_Y_SUB);
    bool is_chopper_low = (chopper_y > (GROUND_Y_SUB - (14 << SUBPIXEL_BITS)));
    int32_t chopper_center_x = chopper_world_x + (16 << SUBPIXEL_BITS);

    int32_t crush_threshold = (current_heading == FACING_CENTER) ? (8<<4) : (15<<4);

    // Check if player is trying to take off
    bool is_taking_off = is_action_pressed(0, ACTION_THRUST);

    // =========================================================
    // 1. SPAWN LOGIC
    // =========================================================
    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        if (base_state[i].destroyed && base_state[i].hostages_remaining > 0) {
            base_state[i].spawn_timer++;
            if (base_state[i].spawn_timer > 45) {
                
                // --- RANDOMIZED SPAWN POINT ---
                // Base Door is roughly at Offset +13. 
                // We add random variance (0-15) to help them spread out immediately.
                // int32_t variance = (rand() % 16) << SUBPIXEL_BITS;
                // int32_t spawn_x = ENEMY_BASE_LOCATIONS[i] + (8 << SUBPIXEL_BITS) + variance;

                bool door_blocked = false;
                int32_t spawn_x = ENEMY_BASE_LOCATIONS[i] + (13 << SUBPIXEL_BITS);

                for (int h = 0; h < NUM_HOSTAGES; h++) {
                    if (hostages[h].state != H_STATE_INACTIVE && 
                        hostages[h].state != H_STATE_ON_BOARD && 
                        hostages[h].state != H_STATE_SAFE) {
                        
                        int32_t h_cx = hostages[h].world_x + (8 << SUBPIXEL_BITS);
                        
                        // REDUCED BLOCK RADIUS:
                        // Changed from 12 to 8. Allows tighter packing at the door.
                        if (labs(h_cx - spawn_x) < (8 << SUBPIXEL_BITS)) {
                            door_blocked = true;
                            break;
                        }
                    }
                }

                if (!door_blocked) {
                    base_state[i].spawn_timer = 0;
                    for (int h = 0; h < NUM_HOSTAGES; h++) {
                        if (hostages[h].state == H_STATE_INACTIVE) {
                            hostages[h].state = H_STATE_RUNNING_CHOPPER;
                            hostages[h].base_id = i;
                            hostages[h].world_x = spawn_x;
                            hostages[h].y = GROUND_Y_SUB + (4 << SUBPIXEL_BITS);
                            hostages[h].anim_frame = 8;
                            hostages[h].direction = 0;
                            base_state[i].hostages_remaining--;
                            hostages_total_spawned++;
                            break;
                        }
                    }
                }
            }
        }
    }

    // =========================================================
    // 2. UNLOAD LOGIC
    // =========================================================
    bool at_home_base = false;
    if (is_chopper_landed) {
        int32_t zone_min = 3920L << SUBPIXEL_BITS;
        int32_t zone_max = 3945L << SUBPIXEL_BITS;
        if (chopper_world_x >= zone_min && chopper_world_x <= zone_max) {
            at_home_base = true;
        }
    }

    if (at_home_base && hostages_on_board > 0) {
        dropoff_timer++;
        if (dropoff_timer > 30) { 
            dropoff_timer = 0;
            for (int h = 0; h < NUM_HOSTAGES; h++) {
                if (hostages[h].state == H_STATE_ON_BOARD) {
                    hostages[h].state = H_STATE_RUNNING_HOME;
                    hostages[h].world_x = chopper_center_x; 
                    hostages[h].base_id = (hostages_on_board == 1) ? 99 : 0; 
                    hostages_on_board--;
                    break;
                }
            }
        }
    }

    // =========================================================
    // 3. HOSTAGE LOOP
    // =========================================================
    for (int i = 0; i < NUM_HOSTAGES; i++) {
        unsigned cfg = HOSTAGE_CONFIG + (i * sizeof(vga_mode4_sprite_t));

        if (hostages[i].state == H_STATE_INACTIVE || 
            hostages[i].state == H_STATE_ON_BOARD || 
            hostages[i].state == H_STATE_SAFE) {
            continue; 
        }

        if (hostages[i].state == H_STATE_DYING) {
            hostages_lost_count++;
            hostages[i].state = H_STATE_INACTIVE;
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
            continue;
        }

        if (is_chopper_low && !is_chopper_landed && !is_taking_off) {
            int32_t host_cx = hostages[i].world_x + (8 << SUBPIXEL_BITS);
            int32_t dist = labs(chopper_center_x - host_cx);
            if (dist < crush_threshold) {
                hostages[i].state = H_STATE_DYING;
                spawn_small_explosion(host_cx, hostages[i].y + (8<<4));
                sfx_hostage_die();
                continue; 
            }
        }

        // --- TARGET SELECTION ---
        int32_t target_x = hostages[i].world_x;
        bool is_moving_state = false;

        if (hostages[i].state == H_STATE_RUNNING_CHOPPER) {
            // Calculate distance to chopper
            int32_t dist_to_chopper = labs(chopper_center_x - hostages[i].world_x);
            
            // Check if Chopper is visible (Landed/Low AND Close)
            bool can_see = (chopper_y >= SIGHT_HEIGHT) && (dist_to_chopper < SIGHT_RANGE);

            if (can_see) {
                // Run to Chopper
                target_x = chopper_center_x;
            } else {
                // WANDER LOGIC UPDATED:
                // Spread them out more so they don't cluster.
                // Modulo 8 allows 8 distinct "waiting spots".
                // Spread: -56 to +56 pixels.
                int32_t base_x = ENEMY_BASE_LOCATIONS[hostages[i].base_id];
                // Scatter offset: -24, -8, +8, +24
                int32_t wander = ((i % 4) * 16 - 24) << SUBPIXEL_BITS;
                target_x = base_x + wander;
            }
            is_moving_state = true;
        }
        else if (hostages[i].state == H_STATE_RUNNING_HOME) {
            target_x = HOMEBASE_WORLD_X + (24 << SUBPIXEL_BITS);
            is_moving_state = true;
        }
        else if (hostages[i].state == H_STATE_WAVING) {
            hostages[i].anim_timer++;
            if (hostages[i].anim_timer > 120) {
                hostages_rescued_count++;
                hostages[i].state = H_STATE_INACTIVE;
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
                sfx_hostage_rescue(); 
                continue;
            }
        }

        // --- PHYSICS & ARRIVAL ---
        int8_t intended_dir = 0;

        if (is_moving_state) {
            int32_t host_cx = hostages[i].world_x + (8 << SUBPIXEL_BITS);
            int32_t diff = target_x - host_cx;
            
            if (labs(diff) > (8 << SUBPIXEL_BITS)) {
                intended_dir = (diff > 0) ? 1 : -1;
            } else {
                // *** ARRIVED AT TARGET ***
                intended_dir = 0; 

                if (hostages[i].state == H_STATE_RUNNING_CHOPPER) {
                    // Only board if we are actually AT the chopper (check distance again)
                    // (If we arrived at the "Wander Point", this check will fail, which is correct)
                    int32_t dist_to_chopper = labs(chopper_center_x - host_cx);
                    
                    if (is_chopper_landed && dist_to_chopper < (12 << SUBPIXEL_BITS) && player_state == PLAYER_ALIVE) {
                        hostages[i].state = H_STATE_ON_BOARD;
                        hostages_on_board++;
                        xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
                        sfx_hostage_rescue();
                        continue;
                    }
                }
                else if (hostages[i].state == H_STATE_RUNNING_HOME) {
                    if (hostages[i].base_id == 99) {
                        hostages[i].state = H_STATE_WAVING;
                        hostages[i].anim_timer = 0;
                    } else {
                        hostages_rescued_count++;
                        hostages[i].state = H_STATE_INACTIVE;
                        xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
                        sfx_hostage_rescue(); 
                        continue;
                    }
                }
            }

            // Spacing Logic
            if (intended_dir != 0) {
                for (int other = 0; other < NUM_HOSTAGES; other++) {
                    if (i == other) continue;
                    if (hostages[other].state == H_STATE_INACTIVE ||
                        hostages[other].state == H_STATE_ON_BOARD || 
                        hostages[other].state == H_STATE_SAFE || 
                        hostages[other].state == H_STATE_DYING ||
                        hostages[other].state == H_STATE_WAVING) continue;

                    int32_t sep = hostages[other].world_x - hostages[i].world_x;
                    if (intended_dir == 1 && sep > 0 && sep < (12<<4)) intended_dir = 0;
                    if (intended_dir == -1 && sep < 0 && sep > -(12<<4)) intended_dir = 0;
                }
            }

            if (intended_dir == 1)  hostages[i].world_x += HOSTAGE_RUN_SPEED;
            if (intended_dir == -1) hostages[i].world_x -= HOSTAGE_RUN_SPEED;
        }

        // --- ANIMATION ---
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