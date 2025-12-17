#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "constants.h"
#include "bomb.h"
#include "player.h"
#include "input.h"
#include "smallexplosion.h"
#include "tanks.h"
#include "explosion.h"
#include "hostages.h"
#include "enemybase.h"

// --- BOMB STATE ---
bool bomb_active = false;
int32_t bomb_world_x = 0;
int32_t bomb_y = 0;       // Using int32 to match world coordinate types
int32_t bomb_target_y = 0;

static bool last_fire_state = false;

void check_tank_collision_bomb(void) {
    const int32_t TANK_HALF_WIDTH = (20 << SUBPIXEL_BITS);

    for (int t = 0; t < NUM_TANKS; t++) {
        if (!tanks[t].active) continue;

        int32_t tank_center = tanks[t].world_x + (20 << SUBPIXEL_BITS);
        int32_t dist = labs(bomb_world_x - tank_center);

        if (dist < TANK_HALF_WIDTH) {
            // DIRECT HIT
            tanks[t].health--;
            
            if (tanks[t].health <= 0) {
                tanks[t].active = false;
                
                // Trigger Big Explosion
                trigger_explosion(tank_center, tanks[t].y - (16 << SUBPIXEL_BITS));
                
                // --- NEW: Set Respawn Cooldown ---
                // Prevent this base from spawning another tank for 3 seconds
                int base_id = tanks[t].base_id;
                if (base_id >= 0 && base_id < NUM_ENEMY_BASES) {
                    base_state[base_id].tank_cooldown = 300; // 300 frames = 5 seconds at 60fps

                    // 2. REPLENISH INVENTORY
                    // This allows the base to spawn a replacement tank later
                    base_state[base_id].tanks_remaining++; 
                }

            } else {
                // Damaged (Optional: Spawn small spark)
                spawn_small_explosion(bomb_world_x, tanks[t].y - (16<<4));
            }
            
            // Hide bomb
            xram0_struct_set(BOMB_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
            return; 
        }
    }
}

void update_bomb(void) {
    
    // =========================================================
    // 1. SPAWN LOGIC
    // =========================================================
    // Trigger: Action Pressed AND Facing Center AND No active bomb
    if (is_action_pressed(0, ACTION_FIRE) && !last_fire_state && !bomb_active) {
        
        if (current_heading == FACING_CENTER) {
            bomb_active = true;
            
            // Spawn at Chopper Center
            // Chopper Center X = WorldX + 16px. Bomb is 8px. Center align: +12px.
            bomb_world_x = chopper_world_x + (12 << SUBPIXEL_BITS);
            
            // Spawn at Chopper Bottom
            bomb_y = chopper_y + (12 << SUBPIXEL_BITS);

            // --- DEPTH LOGIC ---
            // If we are High Up, the bomb falls "Deep" (to the Tank layer)
            // If we are Low, it hits the Ground (Misses tanks)
            if (chopper_y < BOMB_DEPTH_ALTITUDE) {
                bomb_target_y = TARGET_Y_TANKS;
            } else {
                bomb_target_y = TARGET_Y_GROUND;
            }
        }
    }
    // Note: We don't update last_fire_state here because update_bullet() or 
    // update_chopper_state might handle the "Fire" button debounce globally. 
    // If not, uncomment this:
    last_fire_state = is_action_pressed(0, ACTION_FIRE);
    // last_fire_state = fire_pressed; 

    // =========================================================
    // 2. PHYSICS & IMPACT
    // =========================================================
    if (bomb_active) {
        // Fall
        bomb_y += BOMB_SPEED_Y;

        // Check Impact
        if (bomb_y >= bomb_target_y) {
            bomb_active = false;
            
            // 1. Visual Effect
            spawn_small_explosion(bomb_world_x + (4 << SUBPIXEL_BITS), bomb_target_y);

            // 2. Check Collisions (Only if hitting Tank Layer)
            if (bomb_target_y == TARGET_Y_TANKS) {
                check_tank_collision_bomb();
            }
            
            // Hide Sprite
            xram0_struct_set(BOMB_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        }
    }

    // =========================================================
    // 3. RENDER
    // =========================================================
    if (bomb_active) {
        int32_t screen_sub = bomb_world_x - camera_x;
        int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

        if (screen_px > -8 && screen_px < 328) {
            xram0_struct_set(BOMB_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_px);
            xram0_struct_set(BOMB_CONFIG, vga_mode4_sprite_t, y_pos_px, bomb_y >> SUBPIXEL_BITS);
        } else {
            xram0_struct_set(BOMB_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        }
    }
}
