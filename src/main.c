#include <rp6502.h>
#include <stdio.h>
#include <stdbool.h>
#include "constants.h"
#include "input.h"
#include "player.h"


unsigned CHOPPER_CONFIG; // Chopper Sprite Configuration
unsigned CHOPPER_LEFT_CONFIG;  // Chopper Left Sprite Configuration
unsigned CHOPPER_RIGHT_CONFIG; // Chopper Right Sprite Configuration
unsigned SKY_CONFIG;      // Sky Background Configuration
unsigned SKY_MAP_START;  
unsigned SKY_MAP_END;


static void init_graphics(void)
{
    // Initialize graphics here
    xregn(1, 0, 0, 1, 1); // 320x240 (4:3)

    CHOPPER_CONFIG = SPRITE_DATA_END;
    CHOPPER_LEFT_CONFIG  = CHOPPER_CONFIG; // Chopper Left Sprite Configuration
    CHOPPER_RIGHT_CONFIG = CHOPPER_CONFIG + sizeof(vga_mode4_sprite_t); // Chopper Right Sprite Configuration

    // Initialize chopper sprite configuration
    int16_t hardware_xl = chopper_xl >> SUBPIXEL_BITS;
    int16_t hardware_xr = chopper_xr >> SUBPIXEL_BITS;
    int16_t hardware_y = chopper_y >> SUBPIXEL_BITS;
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, x_pos_px, hardware_xl);
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, hardware_y);
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_chopper_sprite_ptr(0, 0));
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, x_pos_px, hardware_xr);
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, hardware_y);
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

    RIA.step0 = 1;

    // Loop Y from Top (0) to Bottom (14)
    for (int y = 0; y < 15; y++) {
        
        // Loop X from Left (0) to Right (19)
        for (int x = 0; x < 20; x++) {
            
            uint8_t tile_id = 0; // Default to Sky (0)

            // --- LAYER 1: GROUND (Bottom 2 Rows) ---
            if (y >= 13) {
                tile_id = 1; // Solid Ground
            }
            
            // --- LAYER 2: MOUNTAINS (Row 12) ---
            else if (y == 12) {
                // Repeat the 8-tile mountain range (8-15)
                // (x % 8) gives 0-7. We add 8 to get tile IDs 8-15.
                tile_id = 8 + (x % 8);
            }
            
            // --- LAYER 3: CLOUDS (Specific Coordinates) ---
            else {
                // Large Cloud 1 (Tiles 2-3) - Place at (x=3, y=3)
                if (y == 3 && x == 3) tile_id = 2;
                if (y == 3 && x == 4) tile_id = 3;

                // Large Cloud 2 (Tiles 4-5) - Place at (x=14, y=5)
                if (y == 5 && x == 14) tile_id = 4;
                if (y == 5 && x == 15) tile_id = 5;

                // Small Cloud (Tile 6) - Place at (x=9, y=2)
                if (y == 2 && x == 9) tile_id = 6;

                // Small Cloud (Tile 7) - Place at (x=18, y=4)
                if (y == 4 && x == 18) tile_id = 7;
            }

            // Write the calculated tile ID to XRAM
            RIA.rw0 = tile_id;
        }
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
    printf("  GAME_PAD_CONFIG=0x%X\n", GAMEPAD_INPUT);
    printf("  KEYBOARD_CONFIG=0x%X\n", KEYBOARD_INPUT);
    printf("  PSG_CONFIG=0x%X\n", PSG_XRAM_ADDR);

}

// void render_game(void)
// {
//     xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, x_pos_px, chopper_xl);
//     xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, x_pos_px, chopper_xr);
//     xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, chopper_y);
//     xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, chopper_y);
// }


uint8_t anim_timer = 0;

int main(void)
{

    puts("Hello from RPMegaChopper");

    init_graphics();

    // Enable keyboard input
    xregn(0, 0, 0, 1, KEYBOARD_INPUT);
    
    // Enable gamepad input
    xregn(0, 0, 2, 1, GAMEPAD_INPUT);

    // Initialize input mappings (ensure `button_mappings` are set)
    init_input_system();  // new function to set up input mappings

    uint8_t vsync_last = RIA.vsync;

    anim_timer = 0;
    uint8_t blade_frame = 0;


    while (1)
    {
        // Main game loop
        // For now, just wait for VBlank
        if (RIA.vsync == vsync_last)
            continue;
        vsync_last = RIA.vsync;

        // Update chopper animation frame
        anim_timer++;
        // if (anim_timer >= 2) // Change frame every 6 VBlanks
        // {
        //     anim_timer = 0;
            
        //     blade_frame = (blade_frame == 0) ? 1 : 0; // Toggle blade frame between 0 and 1

        //     update_chopper_animation(blade_frame);

        // }

        // Handle input
        handle_input();

        // Update player state
        // update_player();
        update_chopper_state();

        // Render the game
        // render_game();

        // Check for ESC key to exit
        if (key(KEY_ESC)) {
            printf("Exiting game...\n");
            break;
        }


    }
}
