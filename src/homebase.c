#include <rp6502.h>
#include <stdint.h>
#include "constants.h"
#include "player.h"
#include "homebase.h"

void update_homebase(void) {
    // Bottom row sits ON the ground (16px high)
    const int16_t ROW0_Y = GROUND_Y; 
    
    for (int i = 0; i < NUM_HOMEBASE_SPRITE; i++) {
        unsigned cfg_addr = HOMEBASE_CONFIG + (i * sizeof(vga_mode4_sprite_t));

        // ---------------------------------------------------
        // 1. CALCULATE LAYOUT
        // ---------------------------------------------------
        int col, row;
        
        if (i < 3) {
            // Bottom Row (0, 1, 2)
            row = 0;
            col = i;
        } else {
            // Top Row (3, 4, 5)
            row = 1;
            col = i - 3; // Reset column index
        }

        int16_t offset_x_px = col * 16;
        // If row 1, move UP by 16 pixels
        int16_t offset_y_px = -(row * 16); 

        // ---------------------------------------------------
        // 2. POSITION & CULL
        // ---------------------------------------------------
        int32_t sprite_world_x = HOMEBASE_WORLD_X + (offset_x_px << SUBPIXEL_BITS);
        int32_t screen_x_sub   = sprite_world_x - camera_x;
        int16_t screen_x_px    = screen_x_sub >> SUBPIXEL_BITS;

        if (screen_x_px > -16 && screen_x_px < 336) {
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, x_pos_px, screen_x_px);
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, (ROW0_Y + offset_y_px));
        } 
        else {
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, -32);
        }
    }
}