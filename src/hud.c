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

void update_hud(void) {
    // 1. Format the string
    // Pad with spaces to clear old numbers. 
    // Format: "LOST: 00  LOAD: 00  SAFE: 00      "
    sprintf(hud_buffer, "     LOST:%02d  LOAD:%02d  SAFE:%02d  ", 
            hostages_lost_count, 
            hostages_on_board, 
            hostages_rescued_count);

    // 2. Write to XRAM
    RIA.addr0 = text_message_addr;
    RIA.step0 = 1;

    for (uint8_t i = 0; i < MESSAGE_WIDTH; i++) {
        char c = hud_buffer[i];
        if (c == 0) c = ' '; // Handle short strings safely

        // Write 3 Bytes per Character (Mode 1, Option 1)
        RIA.rw0 = c;    // Character Index (ASCII)
        RIA.rw0 = 0xFF; // Foreground Color (White/Index 255)
        RIA.rw0 = 0x00; // Background Color (Transparent/Index 0)
    }
}