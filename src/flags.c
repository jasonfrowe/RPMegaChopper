#include <rp6502.h>
#include <stdint.h>
#include "constants.h"
#include "player.h"
#include "homebase.h"
#include "flags.h"


static uint8_t flag_anim_timer = 0;
static uint8_t flag_frame_idx = 0;

void update_flags(void) {
    // ---------------------------------------------------
    // 1. ANIMATION LOGIC
    // ---------------------------------------------------
    // Toggle frame every 10 ticks (approx 6 times a second)
    flag_anim_timer++;
    if (flag_anim_timer > 10) {
        flag_anim_timer = 0;
        flag_frame_idx = !flag_frame_idx; // Toggle 0 -> 1 -> 0
    }

    // Calculate Data Pointer based on frame
    // Frame 0 = Offset 0
    // Frame 1 = Offset 512 (16x16 pixels * 2 bytes)
    uint16_t current_sprite_ptr = FLAGS_DATA + (flag_frame_idx * 512);

    // ---------------------------------------------------
    // 2. POSITIONING
    // ---------------------------------------------------
    // Homebase is 3 sprites wide. We want the CENTER (Index 1).
    // X Offset = 16 pixels.
    int32_t flag_world_x = HOMEBASE_WORLD_X + (32 << SUBPIXEL_BITS);
    
    // Y Position:
    // Ground Y = 186 (example)
    // Homebase is 2 blocks tall (32px).
    // Flag sits on top -> Move up another 16px.
    // Total Height = 48px from ground.
    const int16_t FLAG_Y = GROUND_Y - 27;

    // ---------------------------------------------------
    // 3. SCREEN CALC & UPDATE
    // ---------------------------------------------------
    int32_t screen_x_sub = flag_world_x - camera_x;
    int16_t screen_x_px  = screen_x_sub >> SUBPIXEL_BITS;

    // Check Visibility
    if (screen_x_px > -16 && screen_x_px < 336) {
        // Update Position
        xram0_struct_set(FLAGS_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_x_px);
        xram0_struct_set(FLAGS_CONFIG, vga_mode4_sprite_t, y_pos_px, FLAG_Y);
        
        // Update Animation Frame
        xram0_struct_set(FLAGS_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, current_sprite_ptr);
    } 
    else {
        // Hide
        xram0_struct_set(FLAGS_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    }
}