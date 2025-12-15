#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include "constants.h"
#include "player.h"
#include "enemybase.h"
#include "hostages.h"

// World X locations for the 4 bases (Subpixels)
// Spread them out: 500, 1500, 2500, 3500
int32_t ENEMY_BASE_LOCATIONS[NUM_ENEMY_BASES] = {
    200L  << SUBPIXEL_BITS,
    1200L << SUBPIXEL_BITS,
    2200L << SUBPIXEL_BITS,
    3200L << SUBPIXEL_BITS
};

void update_enemybase(void) {
    const int16_t BASE_Y = GROUND_Y - 16;

    bool visible_base_found = false;

    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        int32_t world_x = ENEMY_BASE_LOCATIONS[i];
        int32_t screen_sub = world_x - camera_x;
        int16_t screen_px  = screen_sub >> SUBPIXEL_BITS;

        // Visibility Check
        if (screen_px > -64 && screen_px < 320) {
            
            // --- CALCULATE POINTERS ---
            // 32x32 sprite = 2048 bytes.
            // Index 0 (Left Intact)  = ENEMYBASE_DATA
            // Index 1 (Right Intact) = ENEMYBASE_DATA + 2048
            // Index 2 (Left Ruined)  = ENEMYBASE_DATA + 4096
            
            uint16_t ptr_left;
            uint16_t ptr_right;

            if (base_state[i].destroyed) {
                // Destroyed: Left uses Index 2, Right keeps Index 1
                ptr_left  = (uint16_t)(ENEMYBASE_DATA + 4096); 
                ptr_right = (uint16_t)(ENEMYBASE_DATA + 2048); 
            } else {
                // Intact: Left uses Index 0, Right uses Index 1
                ptr_left  = (uint16_t)(ENEMYBASE_DATA);
                ptr_right = (uint16_t)(ENEMYBASE_DATA + 2048);
            }

            // --- UPDATE HARDWARE ---
            
            // Left Sprite
            unsigned cfg_left = ENEMYBASE_CONFIG;
            xram0_struct_set(cfg_left, vga_mode4_sprite_t, x_pos_px, screen_px);
            xram0_struct_set(cfg_left, vga_mode4_sprite_t, y_pos_px, BASE_Y);
            xram0_struct_set(cfg_left, vga_mode4_sprite_t, xram_sprite_ptr, ptr_left);

            // Right Sprite
            unsigned cfg_right = ENEMYBASE_CONFIG + sizeof(vga_mode4_sprite_t);
            xram0_struct_set(cfg_right, vga_mode4_sprite_t, x_pos_px, (screen_px + 32));
            xram0_struct_set(cfg_right, vga_mode4_sprite_t, y_pos_px, BASE_Y);
            xram0_struct_set(cfg_right, vga_mode4_sprite_t, xram_sprite_ptr, ptr_right);

            visible_base_found = true;
            break;
        }
    }

    if (!visible_base_found) {
        xram0_struct_set(ENEMYBASE_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(ENEMYBASE_CONFIG + sizeof(vga_mode4_sprite_t), vga_mode4_sprite_t, y_pos_px, -32);
    }
}