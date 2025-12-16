#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "constants.h"
#include "hud.h"
#include "hostages.h"

char message[MESSAGE_LENGTH];

char hud_buffer[MESSAGE_WIDTH + 1]; // +1 for null terminator

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
    draw_hud_stat(10, ICON_DEAD_SPLAT, HUD_COL_RED, hostages_lost_count);

    // Offset 12: LOAD (Yellow)
    draw_hud_stat(16, ICON_FACE_FILLED, HUD_COL_YELLOW, hostages_on_board);

    // Offset 22: SAFE (Green)
    draw_hud_stat(22, ICON_HOUSE, HUD_COL_GREEN, hostages_rescued_count);
}