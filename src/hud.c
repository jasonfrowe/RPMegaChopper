#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "constants.h"
#include "hud.h"
#include "hostages.h"
#include "player.h"

char message[MESSAGE_LENGTH];

char hud_buffer[MESSAGE_WIDTH + 1]; // +1 for null terminator

// Centered roughly on Row 6 (below HUD, above ground)
#define SORTIE_MSG_Y 6 

// Helper function: Center text horizontally (40-char width)
void center_text(uint8_t y, const char* str, uint8_t color) {
    uint8_t len = strlen(str);
    uint8_t x = (40 - len) / 2;
    draw_text(x, y, str, color);
}

// For variable-length strings (e.g., stats)
void center_text_buf(uint8_t y, char* buf, uint8_t color) {
    uint8_t len = strlen(buf);
    uint8_t x = (40 - len) / 2;
    draw_text(x, y, buf, color);
}

void draw_sortie_message(int lives_left) {
    const char* msg;
    
    // Map lives to string
    // Lives = 3 -> First
    // Lives = 2 -> Second
    // Lives = 1 -> Third
    switch(lives_left) {
        case 3: msg = "- FIRST SORTIE -"; break;
        case 2: msg = "- SECOND SORTIE -"; break;
        case 1: msg = "- THIRD SORTIE -"; break;
        default: msg = "- SORTIE -"; break;
    }
    
    // Calculate Center X (Screen is 40 chars wide)
    // strlen is approx 16. 40-16 = 24 / 2 = 12.
    // We use a hardcoded X for simplicity.
    uint8_t x = 12; 
    
    draw_text(x, SORTIE_MSG_Y, msg, HUD_COL_YELLOW);
}

void clear_sortie_message(void) {
    // Overwrite with spaces
    draw_text(12, SORTIE_MSG_Y, "                 ", HUD_COL_BG);
}

void update_lives_display(void) {
    for (int i = 0; i < LIVES_STARTING; i++) {
        unsigned cfg = MINICHOPPER_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        
        // Only draw sprites for lives we HAVE (excluding current one usually, or including?)
        // Let's show "Spare Lives" (Lives - 1).
        if (i < (lives - 1)) {
            xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, (8 + (i * 10)));
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, 220); // Bottom of screen
        } else {
            xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -32); // Hide
        }
    }
}

// Clear all 15 rows of text
void clear_text_screen(void) {
    RIA.addr0 = text_message_addr;
    RIA.step0 = 1;
    
    // Fill 40x15 chars with 0 (Transparent)
    for (int i = 0; i < (MESSAGE_WIDTH * MESSAGE_HEIGHT * 3); i++) {
        RIA.rw0 = 0;
    }
}

// Draw string at x,y (Grid coords 0-39, 0-14)
void draw_text(uint8_t x, uint8_t y, const char* str, uint8_t color) {
    // Calculate offset: (y * width + x) * 3 bytes
    unsigned offset = ((y * MESSAGE_WIDTH) + x) * 3;
    unsigned addr = text_message_addr + offset;

    RIA.addr0 = addr;
    RIA.step0 = 1;

    while (*str) {
        RIA.rw0 = *str++;    // Char
        RIA.rw0 = color;     // FG
        RIA.rw0 = 0;         // BG
    }
}

// void draw_ansi_art(uint8_t x, uint8_t y, const char* lines[], uint8_t num_lines, uint8_t color) {
//     for (uint8_t i = 0; i < num_lines; i++) {
//         draw_text(x, y + i, lines[i], color);
//     }
// }

// Helper to draw: [ICON] [:] [00]
void draw_hud_stat(uint8_t offset, uint8_t icon, uint8_t color, int value) {
    
    // 1. Calculate XRAM Address (3 bytes per char)
    unsigned addr = text_message_addr + (offset * 3);
    
    RIA.addr0 = addr;
    RIA.step0 = 1;

    // 2. Draw Icon (Colored)
    RIA.rw0 = icon;
    RIA.rw0 = color;      // Foreground Index
    RIA.rw0 = HUD_COL_BG; // Background Index

    // 3. Draw Colon ' :' (White)
    RIA.rw0 = ':';
    RIA.rw0 = HUD_COL_WHITE;
    RIA.rw0 = HUD_COL_BG;

    // 4. Draw Numbers (White)
    uint8_t tens = (value / 10) % 10;
    uint8_t ones = value % 10;

    RIA.rw0 = '0' + tens;
    RIA.rw0 = HUD_COL_WHITE;
    RIA.rw0 = HUD_COL_BG;

    RIA.rw0 = '0' + ones;
    RIA.rw0 = HUD_COL_WHITE;
    RIA.rw0 = HUD_COL_BG;
}

void update_hud(void) {
    // Offset 2: LOST (Red)
    draw_hud_stat(12, ICON_DEAD_SPLAT, HUD_COL_RED, hostages_lost_count);

    // Offset 12: LOAD (Yellow)
    draw_hud_stat(18, ICON_FACE_FILLED, HUD_COL_YELLOW, hostages_on_board);

    // Offset 22: SAFE (Green)
    draw_hud_stat(24, ICON_HOUSE, HUD_COL_GREEN, hostages_rescued_count);
}