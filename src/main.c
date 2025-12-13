#include <rp6502.h>
#include <stdio.h>
#include <stdbool.h>
#include "constants.h"


unsigned CHOPPER_CONFIG; // Chopper Sprite Configuration
unsigned CHOPPER_LEFT_CONFIG;  // Chopper Left Sprite Configuration
unsigned CHOPPER_RIGHT_CONFIG; // Chopper Right Sprite Configuration
unsigned SKY_CONFIG;      // Sky Background Configuration
unsigned SKY_MAP_START;  
unsigned SKY_MAP_END;

// Helper to calculate the memory offset for a specific animation frame
// frame_idx: 0 to 21
// part: 0 = Left Half, 1 = Right Half
uint16_t get_chopper_sprite_ptr(int frame_idx, int part) {
    // Each 16x16 block is 512 bytes (256 pixels * 2 bytes)
    // Each Full Frame (Left+Right) is 1024 bytes
    uint16_t frame_offset = frame_idx * 1024;
    uint16_t part_offset = part * 512;
    return CHOPPER_DATA + frame_offset + part_offset;
}


// ===============================================================
// Global Game State Variables
// ===============================================================

int16_t chopper_xl = SCREEN_WIDTH / 2;
int16_t chopper_xr = SCREEN_WIDTH / 2 + 16;
int16_t chopper_y = SCREEN_HEIGHT / 2;
int16_t chopper_frame = 0; // Current frame index (0-21)


static void init_graphics(void)
{
    // Initialize graphics here
    xregn(1, 0, 0, 1, 1); // 320x240 (4:3)

    CHOPPER_CONFIG = SPRITE_DATA_END;
    CHOPPER_LEFT_CONFIG  = CHOPPER_CONFIG; // Chopper Left Sprite Configuration
    CHOPPER_RIGHT_CONFIG = CHOPPER_CONFIG + sizeof(vga_mode4_sprite_t); // Chopper Right Sprite Configuration

    // Initialize chopper sprite configuration
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, x_pos_px, chopper_xl);
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, chopper_y);
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_chopper_sprite_ptr(0, 0));
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, x_pos_px, chopper_xr);
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, chopper_y);
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_chopper_sprite_ptr(0, 1));
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    xregn(1, 0, 1, 5, 4, 0, CHOPPER_LEFT_CONFIG, 2, 2); // Enable sprite


    SKY_MAP_START = CHOPPER_CONFIG + 3 * sizeof(vga_mode4_sprite_t); // Sky Background Configuration
    SKY_MAP_END   = (SKY_MAP_START + SKY_MAP_SIZE);

    // -----------------------------------------------------
    // 3. FILL SKY MAP (Plane 2 - Data)
    // -----------------------------------------------------

    RIA.addr0 = SKY_MAP_START;

    // RIA.addr0 = SKY_MAP_START & 0xFF; // Extracts the Low Byte (0x90)
    // RIA.addr1 = SKY_MAP_START >> 8;   // Extracts the High Byte (0x58)

    RIA.step0 = 1;

    for(int i=0; i<300; i++) {
        RIA.rw0 = 0; // Write 8-bit Tile ID 0
    }

    SKY_CONFIG = SKY_MAP_END;

    // -----------------------------------------------------
    // 2. CONFIGURE PLANE 2 (The Background)
    // -----------------------------------------------------
    // We write to the address SKY_CONFIG


    xram0_struct_set(SKY_CONFIG, vga_mode2_config_t, x_wrap, true);
    xram0_struct_set(SKY_CONFIG, vga_mode2_config_t, y_wrap, true);
    xram0_struct_set(SKY_CONFIG, vga_mode2_config_t, x_pos_px, 0);
    xram0_struct_set(SKY_CONFIG, vga_mode2_config_t, y_pos_px, 0);
    xram0_struct_set(SKY_CONFIG, vga_mode2_config_t, width_tiles, 20);
    xram0_struct_set(SKY_CONFIG, vga_mode2_config_t, height_tiles, 15);
    xram0_struct_set(SKY_CONFIG, vga_mode2_config_t, xram_data_ptr, SKY_MAP_START);
    xram0_struct_set(SKY_CONFIG, vga_mode2_config_t, xram_palette_ptr, 0xFFFF);
    xram0_struct_set(SKY_CONFIG, vga_mode2_config_t, xram_tile_ptr, SKY_DATA);


    // Enable Plane 2 (Background) at Register 9
    // Args: dev(1), chan(0), reg(9), count(3), mode(2), options(0), config_addr
    // xregn(1, 0, 9, 3, 2, 10, SKY_CONFIG); 
    xregn(1, 0, 1, 4, 2, 10, SKY_CONFIG, 0); // Enable sprite

    printf("Chopper Data at 0x%04X\n", CHOPPER_DATA);
    printf("Sky Data at 0x%04X\n", SKY_DATA);
    printf("Chopper Left Sprite Config at 0x%04X\n", CHOPPER_LEFT_CONFIG);
    printf("Chopper Right Sprite Config at 0x%04X\n", CHOPPER_RIGHT_CONFIG);
    printf("Sky Map Start at 0x%04X\n", SKY_MAP_START);
    printf("Sky Map End at 0x%04X\n", SKY_MAP_END);
    printf("Sky Background Config at 0x%04X\n", SKY_CONFIG);

}


int main(void)
{
    init_graphics();
    puts("Hello from RPMegaChopper");

    uint8_t vsync_last = RIA.vsync;
    while (1)
    {
        // Main game loop
        // For now, just wait for VBlank
        if (RIA.vsync == vsync_last)
            continue;
        vsync_last = RIA.vsync;
    }
}
