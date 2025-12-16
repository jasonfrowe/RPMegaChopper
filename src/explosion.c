#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include "constants.h"
#include "explosion.h"
#include "player.h"

// --- EXPLOSION STATE ---
bool exp_active = false;
int32_t exp_world_x = 0;
int16_t exp_y = 0;
uint8_t exp_frame = 0; // 0 to 4 (5 frames total)
uint8_t exp_timer = 0;

// Helper to get Explosion Sprite Data Pointer
// Frame 0: Indices 0,1 (Offset 0)
// Frame 1: Indices 2,3 (Offset 1024) - Each 16x16 is 512 bytes. 2 sprites = 1024 bytes per frame.
uint16_t get_explosion_ptr(int frame_idx) {
    return EXPLOSION_DATA + (frame_idx * 1024);
}

// Call this when the bullet hits the base
void trigger_explosion(int32_t x, int16_t y) {
    exp_active = true;
    exp_world_x = x;
    exp_y = y;
    exp_frame = 0;
    exp_timer = 0;
}

// Call this in your main loop
void update_explosion(void) {
    if (!exp_active) {
        // Hide sprites offscreen
        xram0_struct_set(EXPLOSION_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(EXPLOSION_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        return;
    }

    // --- ANIMATION ---
    exp_timer++;
    if (exp_timer > 8) { // Speed of explosion
        exp_timer = 0;
        exp_frame++;
        if (exp_frame > 4) { // 5 Frames (0-4)
            exp_active = false;
            return;
        }
    }

    // --- RENDER ---
    // Calculate Screen Position
    int32_t screen_sub = exp_world_x - camera_x;
    int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

    // Data Pointers
    // Left Half uses the calculated ptr
    // Right Half uses ptr + 512 (next 16x16 block)
    uint16_t base_ptr = get_explosion_ptr(exp_frame);

    // Left Sprite
    xram0_struct_set(EXPLOSION_LEFT_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_px);
    xram0_struct_set(EXPLOSION_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, exp_y >> SUBPIXEL_BITS);
    xram0_struct_set(EXPLOSION_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, base_ptr);

    // Right Sprite
    xram0_struct_set(EXPLOSION_RIGHT_CONFIG, vga_mode4_sprite_t, x_pos_px, (screen_px + 16));
    xram0_struct_set(EXPLOSION_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, exp_y >> SUBPIXEL_BITS);
    xram0_struct_set(EXPLOSION_RIGHT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (base_ptr + 512));
}
