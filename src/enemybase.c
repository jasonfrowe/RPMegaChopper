#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include "constants.h"
#include "player.h"
#include "enemybase.h"

// World X locations for the 4 bases (Subpixels)
// Spread them out: 500, 1500, 2500, 3500
int32_t ENEMY_BASE_LOCATIONS[NUM_ENEMY_BASES] = {
    200L  << SUBPIXEL_BITS,
    1200L << SUBPIXEL_BITS,
    2200L << SUBPIXEL_BITS,
    3200L << SUBPIXEL_BITS
};

void update_enemybase(void) {
    // The base is 32 pixels tall and sits on the ground.
    // GROUND_Y is 186.
    // Y Position = 186 - 32 = 154.
    const int16_t BASE_Y = GROUND_Y - 16;

    bool visible_base_found = false;

    // Loop through all potential base locations
    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        
        int32_t world_x = ENEMY_BASE_LOCATIONS[i];
        
        // Calculate Screen Position
        int32_t screen_sub = world_x - camera_x;
        int16_t screen_px  = screen_sub >> SUBPIXEL_BITS;

        // Check Visibility
        // Base width is 64 pixels (32 + 32).
        // Visible if Right Edge > 0 AND Left Edge < 320
        if (screen_px > -64 && screen_px < 320) {
            
            // Found a visible base! Position the hardware sprites here.
            
            // --- Left Sprite (0) ---
            unsigned cfg_left = ENEMYBASE_CONFIG; // Index 0
            xram0_struct_set(cfg_left, vga_mode4_sprite_t, x_pos_px, screen_px);
            xram0_struct_set(cfg_left, vga_mode4_sprite_t, y_pos_px, BASE_Y);

            // --- Right Sprite (1) ---
            unsigned cfg_right = ENEMYBASE_CONFIG + sizeof(vga_mode4_sprite_t); // Index 1
            xram0_struct_set(cfg_right, vga_mode4_sprite_t, x_pos_px, (screen_px + 32));
            xram0_struct_set(cfg_right, vga_mode4_sprite_t, y_pos_px, BASE_Y);

            visible_base_found = true;
            break; // Stop searching, we can only see one base at a time
        }
    }

    // If no bases are on screen, hide the sprites
    if (!visible_base_found) {
        unsigned cfg_left = ENEMYBASE_CONFIG;
        unsigned cfg_right = ENEMYBASE_CONFIG + sizeof(vga_mode4_sprite_t);
        
        xram0_struct_set(cfg_left, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(cfg_right, vga_mode4_sprite_t, y_pos_px, -32);
    }
}