#include <rp6502.h>
#include <stdint.h>
#include "constants.h"
#include "player.h"
#include "landing.h"


void update_landing(void) {
    // It sits ON the ground.
    // Height is 1 sprite (16 pixels).
    const int16_t BASE_Y = GROUND_Y + 6; 

    for (int i = 0; i < NUM_LANDING_PAD_SPRITE; i++) {
        unsigned cfg_addr = LANDINGPAD_CONFIG + (i * sizeof(vga_mode4_sprite_t));

        // ---------------------------------------------------
        // 1. CALCULATE LAYOUT (Horizontal Strip)
        // ---------------------------------------------------
        // i = 0, 1, 2, ... 8
        int16_t offset_x_px = i * 16;

        // ---------------------------------------------------
        // 2. CALCULATE SCREEN POSITION
        // ---------------------------------------------------
        // World Pos = Base + Offset
        int32_t sprite_world_x = LANDING_PAD_WORLD_X + (offset_x_px << SUBPIXEL_BITS);
        
        // Screen Pos = World - Camera
        // Note: Make sure to subtract camera first before shifting down to pixels
        int32_t screen_x_sub = sprite_world_x - camera_x;
        int16_t screen_x_px = screen_x_sub >> SUBPIXEL_BITS;

        // ---------------------------------------------------
        // 3. CULLING & UPDATE
        // ---------------------------------------------------
        // Check if visible (-16 to 336 pixels)
        // 320 screen width + 16 buffer
        if (screen_x_px > -16 && screen_x_px < 336) {
            // Visible: Draw at correct X and fixed Ground Y
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, x_pos_px, screen_x_px);
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, BASE_Y);
        } 
        else {
            // Hidden: Move offscreen vertically
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, -32);
        }
    }
}