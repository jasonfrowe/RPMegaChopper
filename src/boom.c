#include <rp6502.h>
#include <stdbool.h>
#include <stdint.h>
#include "boom.h"
#include "constants.h" // Must define BOOM_CONFIG and BOOM_DATA

// --- STATE ---
static bool boom_active = false;
static uint8_t boom_timer = 0;

// --- PUBLIC FUNCTIONS ---

void trigger_boom(int16_t screen_x, int16_t screen_y) {
    boom_active = true;
    boom_timer = 0;

    // Set Position
    xram0_struct_set(BOOM_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_x);
    xram0_struct_set(BOOM_CONFIG, vga_mode4_sprite_t, y_pos_px, screen_y);
    
    // Set Frame 0 (Start)
    xram0_struct_set(BOOM_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (uint16_t)BOOM_DATA);
}

void update_boom(void) {
    if (!boom_active) return;

    boom_timer++;

    // Switch to Frame 1 after 5 ticks
    if (boom_timer == 5) {
        // Frame 1 is offset by 512 bytes (16x16 * 2)
        xram0_struct_set(BOOM_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (uint16_t)(BOOM_DATA + 512));
    }
    // End animation after 10 ticks
    else if (boom_timer >= 10) {
        boom_active = false;
        xram0_struct_set(BOOM_CONFIG, vga_mode4_sprite_t, y_pos_px, -32); // Hide
    }
}

void reset_boom(void) {
    boom_active = false;
    xram0_struct_set(BOOM_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
}