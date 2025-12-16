#include <rp6502.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "constants.h"
#include "input.h"
#include "player.h"
#include "clouds.h"
#include "landing.h"
#include "homebase.h"
#include "flags.h"
#include "enemybase.h"
#include "bullets.h"
#include "hostages.h"
#include "hud.h"
#include "explosion.h"
#include "smallexplosion.h"
#include "tanks.h"
#include "ebullets.h"


unsigned CHOPPER_CONFIG;            // Chopper Sprite Configuration
unsigned CHOPPER_LEFT_CONFIG;       // Chopper Left Sprite Configuration
unsigned CHOPPER_RIGHT_CONFIG;      // Chopper Right Sprite Configuration
unsigned GROUND_CONFIG;             // Ground Background Configuration
unsigned GROUND_MAP_START;          // Ground Background Configuration
unsigned GROUND_MAP_END;
unsigned CLOUD_A_CONFIG;            // Cloud A Sprite Configuration
unsigned CLOUD_B_CONFIG;            // Cloud B Sprite Configuration
unsigned CLOUD_C_CONFIG;            // Cloud C Sprite Configuration
unsigned LANDINGPAD_CONFIG;         // Landing Pad Sprite Configuration
unsigned HOMEBASE_CONFIG;           // Home Base Sprite Configuration
unsigned ENEMYBASE_CONFIG;          // Enemy Base Sprite Configuration
unsigned FLAGS_CONFIG;              // Flags Sprite Configuration
unsigned BULLET_CONFIG;             // Bullet Sprite Configuration
unsigned HOSTAGE_CONFIG;            // Hostage Sprite Configuration
unsigned TEXT_CONFIG;               // Text Plane Configuration
unsigned text_message_addr;         // Text message address
unsigned EXPLOSION_LEFT_CONFIG;     // Explosion Sprite Configuration
unsigned EXPLOSION_RIGHT_CONFIG;    // Explosion Sprite Configuration
unsigned SMALL_EXPLOSION_CONFIG;    // Small Explosion Sprite Configuration
unsigned TANK_CONFIG;               // Tank Sprite Configuration
unsigned EBULLET_CONFIG;            // Enemy Bullet Sprite Configuration


static void init_graphics(void)
{
    // Initialize graphics here
    xregn(1, 0, 0, 1, 1); // 320x240 (4:3)

    uint16_t tile_palette[16] = {
        0x0020,  // Index 0 (Transparent)
        0x07E0,
        0xA8E6,
        0xEB2F,
        0xFFE0,
        0xFFFF,
        0xF4A7,
        0x4620,
        0x0020,
        0x0020,
        0x0020,
        0x0020,
        0x0020,
        0x0020,
        0x0020,
        0x0020,
    };

    RIA.addr0 = PALETTE_ADDR;
    RIA.step0 = 1;
    for (int i = 0; i < 16; i++) {
        // Write Low Byte, then High Byte
        RIA.rw0 = tile_palette[i] & 0xFF;
        RIA.rw0 = tile_palette[i] >> 8;
    }

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


    // Add in HOSTAGES
    HOSTAGE_CONFIG = CHOPPER_RIGHT_CONFIG + sizeof(vga_mode4_sprite_t);
    for (int i = 0; i < NUM_HOSTAGES; i++) {
        unsigned hostage_cfg = HOSTAGE_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        xram0_struct_set(hostage_cfg, vga_mode4_sprite_t, x_pos_px, -16); // Off-screen initially
        xram0_struct_set(hostage_cfg, vga_mode4_sprite_t, y_pos_px, -16);
        xram0_struct_set(hostage_cfg, vga_mode4_sprite_t, xram_sprite_ptr, HOSTAGES_DATA); // Each hostage is 16x16 (512 bytes)
        xram0_struct_set(hostage_cfg, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
        xram0_struct_set(hostage_cfg, vga_mode4_sprite_t, has_opacity_metadata, false);
    }

    // Add in BULLET
    BULLET_CONFIG = HOSTAGE_CONFIG + (NUM_HOSTAGES * sizeof(vga_mode4_sprite_t));
    xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, x_pos_px, -8); // Off-screen initially
    xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, y_pos_px, -8);
    xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, BULLET_DATA); // Bullet sprite data
    xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, log_size, 1);  // 2x2 sprite (2^1)
    xram0_struct_set(BULLET_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    // Add in EXPLOSION
    EXPLOSION_LEFT_CONFIG = BULLET_CONFIG + sizeof(vga_mode4_sprite_t);
    xram0_struct_set(EXPLOSION_LEFT_CONFIG, vga_mode4_sprite_t, x_pos_px, -16); // Off-screen initially
    xram0_struct_set(EXPLOSION_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, -16);
    xram0_struct_set(EXPLOSION_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, EXPLOSION_DATA); // Explosion sprite data
    xram0_struct_set(EXPLOSION_LEFT_CONFIG, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
    xram0_struct_set(EXPLOSION_LEFT_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    EXPLOSION_RIGHT_CONFIG = EXPLOSION_LEFT_CONFIG + sizeof(vga_mode4_sprite_t);
    xram0_struct_set(EXPLOSION_RIGHT_CONFIG, vga_mode4_sprite_t, x_pos_px, -16); // Off-screen initially
    xram0_struct_set(EXPLOSION_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, -16);
    xram0_struct_set(EXPLOSION_RIGHT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (EXPLOSION_DATA + 512)); // Explosion sprite data (right half)
    xram0_struct_set(EXPLOSION_RIGHT_CONFIG, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
    xram0_struct_set(EXPLOSION_RIGHT_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    SMALL_EXPLOSION_CONFIG = EXPLOSION_RIGHT_CONFIG + sizeof(vga_mode4_sprite_t);
    for (uint8_t i = 0; i < MAX_EXPLOSIONS; i++) {
        unsigned ptr = SMALL_EXPLOSION_CONFIG + i * sizeof(vga_mode4_sprite_t);
        xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, -8); // Off-screen initially
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, -8);
        xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, SMALL_EXPLOSION_DATA);
        xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, 3);  // 8x8 sprite (2^3)
        xram0_struct_set(ptr, vga_mode4_sprite_t, has_opacity_metadata, false);
    }

    TANK_CONFIG = SMALL_EXPLOSION_CONFIG + (MAX_EXPLOSIONS * sizeof(vga_mode4_sprite_t));
   
     // Configure all Tank Sprites
    int total_tank_sprites = NUM_TANKS * SPRITES_PER_TANK; // 18
    
    for (int i = 0; i < total_tank_sprites; i++) {
        unsigned cfg = TANK_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        
        xram0_struct_set(cfg, vga_mode4_sprite_t, x_pos_px, -TANK_WIDTH_PX); // Off-screen initially
        xram0_struct_set(cfg, vga_mode4_sprite_t, y_pos_px, -TANK_HEIGHT_PX);
        // Each tank sprite is 8x8 (128 bytes)
        xram0_struct_set(cfg, vga_mode4_sprite_t, xram_sprite_ptr, (TANK_DATA + (i * 128)));
        xram0_struct_set(cfg, vga_mode4_sprite_t, log_size, 3);  // 8x8 sprite (2^3)
        xram0_struct_set(cfg, vga_mode4_sprite_t, has_opacity_metadata, false);
    }

    // Initialize Logical State
    for (int t = 0; t < NUM_TANKS; t++) {
        tanks[t].active = false;
        tanks[t].world_x = TANK_SPAWNS[t];
        tanks[t].y = GROUND_Y_SUB + (32 << SUBPIXEL_BITS); // Sit on ground
        tanks[t].direction = -1; // Move Left initially
        tanks[t].health = 3;
    }

    EBULLET_CONFIG = TANK_CONFIG + (total_tank_sprites * sizeof(vga_mode4_sprite_t));

    for (int i = 0; i < NEBULLET; i++) {
        unsigned ebullet_cfg = EBULLET_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        xram0_struct_set(ebullet_cfg, vga_mode4_sprite_t, x_pos_px, -8); // Off-screen initially
        xram0_struct_set(ebullet_cfg, vga_mode4_sprite_t, y_pos_px, -8); 
        xram0_struct_set(ebullet_cfg, vga_mode4_sprite_t, xram_sprite_ptr, BULLET_DATA); // Enemy Bullet sprite data -- same as player bullet
        xram0_struct_set(ebullet_cfg, vga_mode4_sprite_t, log_size, 1);  // 2x2 sprite (2^1) 
        xram0_struct_set(ebullet_cfg, vga_mode4_sprite_t, has_opacity_metadata, false);
    }

    xregn(1, 0, 1, 5, 4, 0, CHOPPER_LEFT_CONFIG, 2 + NUM_HOSTAGES + NUM_BULLETS + 2 + MAX_EXPLOSIONS + total_tank_sprites + NEBULLET, 2); // Enable sprites

    unsigned FOREGROUND_SPRITE_END = EBULLET_CONFIG + (NEBULLET * sizeof(vga_mode4_sprite_t));


    // -----------------------------------------------------
    // SETUP CLOUDS
    // -----------------------------------------------------

    for (int i = 0; i < NUM_CLOUDS; i++) {
        // Random depth shift: 1 (fast), 2 (medium), or 3 (slow)
        cloud_depth_shift[i] = i + 1;
        
        // Random Height
        // cloud_y[i] = (rand() % (MAX_CLOUD_Y - MIN_CLOUD_Y)) + MIN_CLOUD_Y;
        int32_t screen_pos = ((i * 60) << SUBPIXEL_BITS) + MIN_CLOUD_Y; // Spread screen positions
        cloud_y[i] = screen_pos;

        // Spread out World X initially (0, 320, 640 approx)
        // screen_pos = (i * 120) << SUBPIXEL_BITS; // Spread screen positions
        screen_pos = (rand() % SCREEN_WIDTH) << SUBPIXEL_BITS;
        cloud_world_x[i] = screen_pos;       // Simple init since camera is 0
        printf("Cloud %d: world_x=%ld, y=%d, depth_shift=%d\n", i, cloud_world_x[i], cloud_y[i], cloud_depth_shift[i]);
    }

    CLOUD_A_CONFIG = FOREGROUND_SPRITE_END;
    CLOUD_B_CONFIG = CLOUD_A_CONFIG + sizeof(vga_mode4_sprite_t);
    CLOUD_C_CONFIG = CLOUD_B_CONFIG + sizeof(vga_mode4_sprite_t);

    xram0_struct_set(CLOUD_A_CONFIG, vga_mode4_sprite_t, x_pos_px, cloud_world_x[0] >> SUBPIXEL_BITS);
    xram0_struct_set(CLOUD_A_CONFIG, vga_mode4_sprite_t, y_pos_px, cloud_y[0] >> SUBPIXEL_BITS);
    xram0_struct_set(CLOUD_A_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, CLOUD_A_DATA);
    xram0_struct_set(CLOUD_A_CONFIG, vga_mode4_sprite_t, log_size, 5);  // 32x32 sprite (2^5)
    xram0_struct_set(CLOUD_A_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    xram0_struct_set(CLOUD_B_CONFIG, vga_mode4_sprite_t, x_pos_px, cloud_world_x[1] >> SUBPIXEL_BITS);
    xram0_struct_set(CLOUD_B_CONFIG, vga_mode4_sprite_t, y_pos_px, cloud_y[1] >> SUBPIXEL_BITS);
    xram0_struct_set(CLOUD_B_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, CLOUD_B_DATA);
    xram0_struct_set(CLOUD_B_CONFIG, vga_mode4_sprite_t, log_size, 5);  // 32x32 sprite (2^5)
    xram0_struct_set(CLOUD_B_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    xram0_struct_set(CLOUD_C_CONFIG, vga_mode4_sprite_t, x_pos_px, cloud_world_x[2] >> SUBPIXEL_BITS);
    xram0_struct_set(CLOUD_C_CONFIG, vga_mode4_sprite_t, y_pos_px, cloud_y[2] >> SUBPIXEL_BITS);
    xram0_struct_set(CLOUD_C_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, CLOUD_C_DATA);
    xram0_struct_set(CLOUD_C_CONFIG, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
    xram0_struct_set(CLOUD_C_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    // SETUP LANDING PAD SPRITE

    LANDINGPAD_CONFIG = CLOUD_C_CONFIG + sizeof(vga_mode4_sprite_t);
    for (int i = 0; i < NUM_LANDING_PAD_SPRITE; i++) {
        unsigned landing_pad_cfg = LANDINGPAD_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        unsigned data_ptr = LANDINGPAD_DATA + (i * 512); // Each landing pad is 16x16 (512 bytes)
        xram0_struct_set(landing_pad_cfg, vga_mode4_sprite_t, x_pos_px, -16); // Off-screen initially
        xram0_struct_set(landing_pad_cfg, vga_mode4_sprite_t, y_pos_px, -16);
        xram0_struct_set(landing_pad_cfg, vga_mode4_sprite_t, xram_sprite_ptr, data_ptr); // Each landing pad is 16x16 (512 bytes)
        xram0_struct_set(landing_pad_cfg, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
        xram0_struct_set(landing_pad_cfg, vga_mode4_sprite_t, has_opacity_metadata, false);
    }

    // SETUP HOMEBASE SPRITE

    HOMEBASE_CONFIG = LANDINGPAD_CONFIG + (NUM_LANDING_PAD_SPRITE * sizeof(vga_mode4_sprite_t));
    for (int i = 0; i < NUM_HOMEBASE_SPRITE; i++) {
        unsigned homebase_cfg = HOMEBASE_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        unsigned data_ptr = HOMEBASE_DATA + (i * 512); // Each home base is 16x16 (512 bytes)
        xram0_struct_set(homebase_cfg, vga_mode4_sprite_t, x_pos_px, -16); // Off-screen initially
        xram0_struct_set(homebase_cfg, vga_mode4_sprite_t, y_pos_px, -16);
        xram0_struct_set(homebase_cfg, vga_mode4_sprite_t, xram_sprite_ptr, data_ptr); // Each home base is 16x16 (512 bytes)
        xram0_struct_set(homebase_cfg, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
        xram0_struct_set(homebase_cfg, vga_mode4_sprite_t, has_opacity_metadata, false);
    }

    // SET UP ENEMY BASE SPRITE

    ENEMYBASE_CONFIG = HOMEBASE_CONFIG + (NUM_HOMEBASE_SPRITE * sizeof(vga_mode4_sprite_t));
    for (int i = 0; i < NUM_ENEMYBASE_SPRITE; i++) {
        unsigned enemybase_cfg = ENEMYBASE_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        unsigned data_ptr = ENEMYBASE_DATA + (i * 2048); // Each enemy base is 32x32 (2048 bytes)
        xram0_struct_set(enemybase_cfg, vga_mode4_sprite_t, x_pos_px, -32); // Off-screen initially
        xram0_struct_set(enemybase_cfg, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(enemybase_cfg, vga_mode4_sprite_t, xram_sprite_ptr, data_ptr); // Each enemy base is 32x32 (2048 bytes)
        xram0_struct_set(enemybase_cfg, vga_mode4_sprite_t, log_size, 5);  // 32x32 sprite (2^5)
        xram0_struct_set(enemybase_cfg, vga_mode4_sprite_t, has_opacity_metadata, false);
    }

    // SET UP FLAG SPRITE
    FLAGS_CONFIG = ENEMYBASE_CONFIG + (NUM_ENEMYBASE_SPRITE * sizeof(vga_mode4_sprite_t));
    xram0_struct_set(FLAGS_CONFIG, vga_mode4_sprite_t, x_pos_px, -16); // Off-screen initially
    xram0_struct_set(FLAGS_CONFIG, vga_mode4_sprite_t, y_pos_px, -16);
    xram0_struct_set(FLAGS_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, FLAGS_DATA);
    xram0_struct_set(FLAGS_CONFIG, vga_mode4_sprite_t, log_size, 4);  // 16x16 sprite (2^4)
    xram0_struct_set(FLAGS_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    xregn(1, 0, 1, 5, 4, 0, CLOUD_A_CONFIG, NUM_CLOUDS + NUM_LANDING_PAD_SPRITE + NUM_HOMEBASE_SPRITE + NUM_ENEMYBASE_SPRITE + NUM_FLAGS, 1); // Enable sprite

    unsigned END_OF_SPRITES = FLAGS_CONFIG + (NUM_FLAGS * sizeof(vga_mode4_sprite_t));

    // Sky Map
    GROUND_MAP_START = END_OF_SPRITES; // Sky Background Configuration
    GROUND_MAP_END   = (GROUND_MAP_START + GROUND_MAP_SIZE);

    // -----------------------------------------------------
    // 3. FILL GROUND MAP (Plane 2 - Data)
    // -----------------------------------------------------

    RIA.addr0 = GROUND_MAP_START;

    RIA.step0 = 1;

    for (int y = 0; y < 15; y++) {
        for (int x = 0; x < 20; x++) {
            uint8_t tile_id = 0; // Default Transparent (if Color 0 is alpha)

            // Ground Logic
            if (y >= 12) {
                tile_id = 1; // Solid Ground
            }
            else if (y == 11) {
                // Mountain Range (2-9)
                tile_id = 2 + (x % 8);
            }
            
            RIA.rw0 = tile_id;
        }
    }

    GROUND_CONFIG = GROUND_MAP_END;

    // // -----------------------------------------------------
    // // 2. CONFIGURE PLANE 2 (The Background)
    // // -----------------------------------------------------
    // // We write to the address GROUND_CONFIG


    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, x_wrap, true);
    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, y_wrap, true);
    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, x_pos_px, 0);
    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, y_pos_px, 0);
    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, width_tiles, 20);
    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, height_tiles, 15);
    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, xram_data_ptr, GROUND_MAP_START);
    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, xram_palette_ptr, PALETTE_ADDR);
    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, xram_tile_ptr, GROUND_DATA);


    // Enable Plane 2 (Background) at Register 9
    // Args: dev(1), chan(0), reg(9), count(3), mode(2), options(0), config_addr
    xregn(1, 0, 1, 4, 2, 10, GROUND_CONFIG, 0); // Enable sprite

    TEXT_CONFIG = GROUND_CONFIG + sizeof(vga_mode2_config_t);
    text_message_addr = TEXT_CONFIG + NTEXT * sizeof(vga_mode1_config_t);
    const unsigned bytes_per_char = 3; // we write 3 bytes per character into text RAM
    unsigned text_storage_end = text_message_addr + MESSAGE_LENGTH * bytes_per_char;

    for (uint8_t i = 0; i < NTEXT; i++) {

        unsigned ptr = TEXT_CONFIG + i * sizeof(vga_mode1_config_t);

        xram0_struct_set(ptr, vga_mode1_config_t, x_wrap, 0);
        xram0_struct_set(ptr, vga_mode1_config_t, y_wrap, 0);
        xram0_struct_set(ptr, vga_mode1_config_t, x_pos_px, 7); //Bug: first char duplicated if not set to zero
        xram0_struct_set(ptr, vga_mode1_config_t, y_pos_px, 1);
        xram0_struct_set(ptr, vga_mode1_config_t, width_chars, MESSAGE_WIDTH);
        xram0_struct_set(ptr, vga_mode1_config_t, height_chars, MESSAGE_HEIGHT);
        xram0_struct_set(ptr, vga_mode1_config_t, xram_data_ptr, text_message_addr);
        xram0_struct_set(ptr, vga_mode1_config_t, xram_palette_ptr, 0xFFFF);
        xram0_struct_set(ptr, vga_mode1_config_t, xram_font_ptr, 0xFFFF);
    }

    // 4 parameters: text mode, 8-bit, config, plane
    xregn(1, 0, 1, 4, 1, 3, TEXT_CONFIG, 2);

    // Clear message buffer to spaces
    for (int i = 0; i < MESSAGE_LENGTH; ++i) message[i] = ' ';

    // Now write the MESSAGE_LENGTH characters into text RAM (3 bytes per char)
    RIA.addr0 = text_message_addr;
    RIA.step0 = 1;
    for (uint8_t i = 0; i < MESSAGE_LENGTH; i++) {
        RIA.rw0 = message[i];
        RIA.rw0 = 0xE0;
        RIA.rw0 = 0x00;
    }

    printf("Next Free XRAM Address: 0x%04X\n", text_storage_end);


    printf("Chopper Data at 0x%04X\n", CHOPPER_DATA);
    printf("Ground Data at 0x%04X\n", GROUND_DATA);
    printf("Cloud A Data at 0x%04X\n", CLOUD_A_DATA);
    printf("Cloud B Data at 0x%04X\n", CLOUD_B_DATA);
    printf("Cloud C Data at 0x%04X\n", CLOUD_C_DATA);
    printf("Chopper Left Sprite Config at 0x%04X\n", CHOPPER_LEFT_CONFIG);
    printf("Chopper Right Sprite Config at 0x%04X\n", CHOPPER_RIGHT_CONFIG);
    printf("Ground Map Start at 0x%04X\n", GROUND_MAP_START);
    printf("Ground Map End at 0x%04X\n", GROUND_MAP_END);
    printf("Ground Background Config at 0x%04X\n", GROUND_CONFIG);
    printf("CLOUD_A_CONFIG=0x%X\n", CLOUD_A_CONFIG);
    printf("CLOUD_B_CONFIG=0x%X\n", CLOUD_B_CONFIG);
    printf("CLOUD_C_CONFIG=0x%X\n", CLOUD_C_CONFIG);
    printf("  LANDINGPAD_CONFIG=0x%X\n", LANDINGPAD_CONFIG);
    printf("  HOMEBASE_CONFIG=0x%X\n", HOMEBASE_CONFIG);
    printf("  ENEMYBASE_CONFIG=0x%X\n", ENEMYBASE_CONFIG);
    printf("END OF SPRITES=0x%X\n", END_OF_SPRITES);
    printf("  GAME_PAD_CONFIG=0x%X\n", GAMEPAD_INPUT);
    printf("  KEYBOARD_CONFIG=0x%X\n", KEYBOARD_INPUT);
    printf("  PSG_CONFIG=0x%X\n", PSG_XRAM_ADDR);

}

void init_game_logic(void) {
    hostages_on_board = 0;
    hostages_rescued_count = 0;

    for (int i = 0; i < NUM_HOSTAGES; i++) {
        hostages[i].state = H_STATE_INACTIVE;
        hostages[i].y = GROUND_Y_SUB - (16 << SUBPIXEL_BITS);
    }

    for (int i = 0; i < NUM_ENEMY_BASES; i++) {
        base_state[i].destroyed = false;
        base_state[i].hostages_remaining = HOSTAGES_PER_BASE;
        base_state[i].spawn_timer = 0;
        base_state[i].tanks_remaining = TANKS_PER_BASE;
    }

    // Reset Active Tank Pool
    for (int t = 0; t < NUM_TANKS; t++) {
        tanks[t].active = false;
    }

}


uint8_t anim_timer = 0;

int main(void)
{

    puts("Hello from RPMegaChopper");

    init_graphics();
    init_game_logic();

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

        // Update animation frames
        anim_timer++;

        // Handle input
        handle_input();

        // Update player state
        update_chopper_state();
        // Update clouds
        update_clouds();
        // Update landing pad
        update_landing();
        // Update home base
        update_homebase();
        // Update flags
        update_flags();
        // Update enemy base
        update_enemybase();
        // Update bullets
        update_bullet();
        // Check bullet collisions
        check_bullet_collisions();
        // Update enemy bullets
        update_tank_bullets();
        // Update hostages
        update_hostages();
        // Update HUD
        update_hud();
        // Update explosion
        update_explosion();
        // Update small explosion
        update_small_explosions();
        // Update tanks
        update_tanks();

        // Render the game
        // render_game();

        // Check for ESC key to exit
        if (key(KEY_ESC)) {
            printf("Exiting game...\n");
            break;
        }


    }
}
