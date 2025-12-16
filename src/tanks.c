#include <rp6502.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "constants.h"
#include "tanks.h"
#include "player.h"


Tank tanks[NUM_TANKS];

// Initial Spawn Locations (Example)
const int32_t TANK_SPAWNS[NUM_TANKS] = {
    1000L << SUBPIXEL_BITS,
    3000L << SUBPIXEL_BITS
};

// Helper to get pointer to specific 16x16 tile index
uint16_t get_tank_tile_ptr(int index) {
    return TANK_DATA + (index * 128);
}

void update_tanks(void) {
    
    for (int t = 0; t < NUM_TANKS; t++) {
        if (!tanks[t].active) {
            // Hide all 9 sprites for this tank
            for(int s=0; s<9; s++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + s) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
            }
            continue;
        }

        // ---------------------------------------------------
        // 1. AI & MOVEMENT
        // ---------------------------------------------------
        // Simple Patrol: Reverse at boundaries or random logic
        // For now, let's just make them drive slowly Left
        tanks[t].world_x += (tanks[t].direction * (TANK_SPEED / 2)); // Half speed

        // Animation Timer (Treads)
        tanks[t].anim_timer++;
        if (tanks[t].anim_timer > 10) {
            tanks[t].anim_timer = 0;
            tanks[t].anim_frame = !tanks[t].anim_frame; // Toggle 0/1
        }

        // ---------------------------------------------------
        // 2. TURRET AIMING
        // ---------------------------------------------------
        // Compare Tank Center X with Chopper Center X
        int32_t tank_cx = tanks[t].world_x + (20 << SUBPIXEL_BITS); // Width 40, Center 20
        int32_t chop_cx = chopper_world_x + (16 << SUBPIXEL_BITS);
        
        // Calculate difference in pixels
        int16_t diff_x = (chop_cx - tank_cx) >> SUBPIXEL_BITS;
        int16_t diff_y = (chopper_y - tanks[t].y) >> SUBPIXEL_BITS; // negative means chopper is above

        if (diff_x > 20) {
            tanks[t].turret_dir = TURRET_RIGHT; // Player is to the Right
        } 
        else if (diff_x < -20) {
            // Player is to the Left
            // We only have Up-Left, Up, Right. Use Up-Left.
            tanks[t].turret_dir = TURRET_UP_LEFT; 
        } 
        else {
            // Player is roughly overhead
            tanks[t].turret_dir = TURRET_UP;
        }

        // ---------------------------------------------------
        // 3. RENDER (THE COMPOSITE SPRITE)
        // ---------------------------------------------------
        int32_t screen_sub = tanks[t].world_x - camera_x;
        int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

        // Culling Check (Tank is 80px wide)
        if (screen_px > -80 && screen_px < 320) {
            
            // --- DRAW BODY (Sprites 0-4) ---
            // Indices: Frame 0 (0-4), Frame 1 (5-9)
            int body_start_idx = (tanks[t].anim_frame == 0) ? 0 : 5;
            int16_t base_y_px = tanks[t].y >> SUBPIXEL_BITS;

            for (int i = 0; i < 5; i++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + i) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, (screen_px + (i * 8)));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, base_y_px);
                xram0_struct_set(cfg, vga_mode4_sprite_t, xram_sprite_ptr, get_tank_tile_ptr(body_start_idx + i));
            }

            // --- DRAW TURRET (Sprites 5-8) ---
            // Width: 4 sprites (64px). 
            // Centered on Body (80px): Offset X = 8px.
            // Height: On top of body: Offset Y = -16px.
            
            int turret_start_idx;
            switch(tanks[t].turret_dir) {
                case TURRET_UP_LEFT: turret_start_idx = 10; break;
                case TURRET_UP:      turret_start_idx = 14; break;
                default:             turret_start_idx = 18; break; // Right
            }

            for (int i = 0; i < 4; i++) {
                // Config indices 5,6,7,8 for this tank
                unsigned cfg = TANK_CONFIG + ((t*9 + (5+i)) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, (screen_px + 4 + (i * 8)));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, (base_y_px - 8));
                xram0_struct_set(cfg, vga_mode4_sprite_t, xram_sprite_ptr, get_tank_tile_ptr(turret_start_idx + i));
            }

        } else {
            // Off-screen: Hide all 9 sprites
            for(int s=0; s<9; s++) {
                unsigned cfg = TANK_CONFIG + ((t*9 + s) * sizeof(vga_mode4_sprite_t));
                xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32);
            }
        }
    }
}