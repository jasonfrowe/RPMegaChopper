#ifndef CONSTANTS_H
#define CONSTANTS_H

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Vertical Boundaries
// Screen is 240 pixels tall.
// Top: Leave room for HUD or status bar?
#define CEILING_Y       20  
#define CEILING_Y_SUB (CEILING_Y << SUBPIXEL_BITS)
// Bottom: 240 - 16 (Chopper Height) - 10 (Margin for ground tiles)
#define GROUND_Y        200 
#define GROUND_Y_SUB (GROUND_Y << SUBPIXEL_BITS)
// Left and Right boundaries can be full width
#define LEFT_BOUNDARY   (SCREEN_WIDTH / 2 - 80)
#define LEFT_BOUNDARY_SUB (LEFT_BOUNDARY << SUBPIXEL_BITS)
#define RIGHT_BOUNDARY  (SCREEN_WIDTH / 2 + 80 - 32) // 32 = Chopper Width
#define RIGHT_BOUNDARY_SUB (RIGHT_BOUNDARY << SUBPIXEL_BITS)

// Scroll Triggers (in Screen Pixels)
#define SCROLL_LEFT_EDGE    (LEFT_BOUNDARY)
#define SCROLL_RIGHT_EDGE   (RIGHT_BOUNDARY)

// World Limits (Optional, for now infinite)
#define WORLD_MIN           0
#define WORLD_MAX           (10000 << SUBPIXEL_BITS)

// XRAM Memory Layout for RPMegaChopper
// ---------------------------------------------------------------------------
// We have a total of 65536 bytes (64KB) of XRAM to work with. We'll allocate
// space for the Chopper sprites, background tiles, maps, and soldier sprites.

// Chopper Sprite Sheet (22 frames, 2x 16x16 sprites per frame)
// Total Size: 22,528 bytes (0x5800)
#define SPRITE_DATA_START       0x0000 // Starting address in XRAM for sprite data
#define CHOPPER_DATA_SIZE       0x5800 // Size of the chopper sprite data
#define CHOPPER_DATA            SPRITE_DATA_START // Chopper sprite data address
#define GROUND_DATA            (SPRITE_DATA_START + CHOPPER_DATA_SIZE) // Ground tile data address
#define GROUND_DATA_SIZE        0x500 // Size of the ground tile data (each is 128 bytes so 10 tiles is 10*128=1280=0x500)
#define CLOUD_A_DATA           (GROUND_DATA + GROUND_DATA_SIZE) // Cloud A sprite data address
#define CLOUD_A_DATA_SIZE       0x0800 // Size of Cloud A sprite data (512 bytes)
#define CLOUD_B_DATA           (CLOUD_A_DATA + CLOUD_A_DATA_SIZE) // Cloud B sprite data address
#define CLOUD_B_DATA_SIZE       0x0800 // Size of Cloud B sprite data (512 bytes)
#define CLOUD_C_DATA           (CLOUD_B_DATA + CLOUD_B_DATA_SIZE) // Cloud C sprite data address
#define CLOUD_C_DATA_SIZE       0x0200 // Size of Cloud C sprite data (256 bytes)

#define SPRITE_DATA_END        (CLOUD_C_DATA + CLOUD_C_DATA_SIZE) // End of used data area

// Helper macros for navigating the Chopper frames
// Each frame consists of a LEFT sprite and a RIGHT sprite
#define SPRITE_SIZE_16x16       512   // Bytes (0x200)
#define CHOPPER_FRAME_STRIDE    1024  // 2 * 16x16 sprites (0x400)

// 2. BACKGROUND / TILES (Planned)
// -------------------------------------------------------------------------
// We start the background data immediately after the chopper to save space.
// 0x0000 + 0x5800 = 0x5800
// #define TILE_DATA_START         (CHOPPER_BASE_ADDR + 0x5800) 

// 3. SPRITE CONFIGURATION
// -------------------------------------------------------------------------
// Configuration data for the chopper sprite
extern unsigned CHOPPER_CONFIG; // Chopper Sprite Configuration
extern unsigned CHOPPER_LEFT_CONFIG; // Chopper Left Sprite Configuration
extern unsigned CHOPPER_RIGHT_CONFIG; // Chopper Right Sprite Configuration
extern unsigned GROUND_CONFIG;      // Ground Background Configuration
extern unsigned CLOUD_A_CONFIG;    // Cloud A Sprite Configuration


// 4. TILE MAP CONFIGURATION
// -------------------------------------------------------------------------
// Configuration data for the ground background
extern unsigned GROUND_MAP_START; // Ground Background Configuration
#define GROUND_MAP_SIZE            0x0258  // 600 bytes
extern unsigned GROUND_MAP_END;

// 5. Keyboard, Gamepad and Sound
// -------------------------------------------------------------------------
#define GAMEPAD_INPUT   0xFF78  // XRAM address for gamepad data
#define KEYBOARD_INPUT  0xFFA0  // XRAM address for keyboard data
#define PSG_XRAM_ADDR   0xFFC0  // PSG memory location (must match sound.c)

// Controller input
#define GAMEPAD_COUNT 4       // Support up to 4 gamepads
#define GAMEPAD_DATA_SIZE 10  // 10 bytes per gamepad
#define KEYBOARD_BYTES  32    // 32 bytes for 256 key states

// Button definitions
#define KEY_ESC 0x29       // ESC key
#define KEY_ENTER 0x28     // ENTER key

// Hardware button bit masks - DPAD
#define GP_DPAD_UP        0x01
#define GP_DPAD_DOWN      0x02
#define GP_DPAD_LEFT      0x04
#define GP_DPAD_RIGHT     0x08
#define GP_SONY           0x40  // Sony button faces (Circle/Cross/Square/Triangle)
#define GP_CONNECTED      0x80  // Gamepad is connected

// Hardware button bit masks - ANALOG STICKS
#define GP_LSTICK_UP      0x01
#define GP_LSTICK_DOWN    0x02
#define GP_LSTICK_LEFT    0x04
#define GP_LSTICK_RIGHT   0x08
#define GP_RSTICK_UP      0x10
#define GP_RSTICK_DOWN    0x20
#define GP_RSTICK_LEFT    0x40
#define GP_RSTICK_RIGHT   0x80

// Hardware button bit masks - BTN0 (Face buttons and shoulders)
// Per RP6502 documentation: https://picocomputer.github.io/ria.html#gamepads
#define GP_BTN_A          0x01  // bit 0: A or Cross
#define GP_BTN_B          0x02  // bit 1: B or Circle
#define GP_BTN_C          0x04  // bit 2: C or Right Paddle
#define GP_BTN_X          0x08  // bit 3: X or Square
#define GP_BTN_Y          0x10  // bit 4: Y or Triangle
#define GP_BTN_Z          0x20  // bit 5: Z or Left Paddle
#define GP_BTN_L1         0x40  // bit 6: L1
#define GP_BTN_R1         0x80  // bit 7: R1

// Hardware button bit masks - BTN1 (Triggers and special buttons)
#define GP_BTN_L2         0x01  // bit 0: L2
#define GP_BTN_R2         0x02  // bit 1: R2
#define GP_BTN_SELECT     0x04  // bit 2: Select/Back
#define GP_BTN_START      0x08  // bit 3: Start/Menu
#define GP_BTN_HOME       0x10  // bit 4: Home button
#define GP_BTN_L3         0x20  // bit 5: L3
#define GP_BTN_R3         0x40  // bit 6: R3

// --- PHYSICS CONSTANTS ---
#define SUBPIXEL_BITS   4       // 2^4 = 16
#define ONE_PIXEL       (1 << SUBPIXEL_BITS) // Value 16

// Chopper animation frames
// LEFT FACING
#define FRAME_LEFT_IDLE         0   // 0-1
#define FRAME_TRANS_L_C         2   // 2-3: Turning Left->Center
#define FRAME_LEFT_ACCEL        10  // 10-11: Facing Left, Moving Left (Forward)
#define FRAME_LEFT_BRAKE        12  // 12-13: Facing Left, Moving Right (Backwards)

// CENTER FACING
#define FRAME_CENTER_IDLE       4   // 4-5
#define FRAME_BANK_LEFT         14  // 14-15: Facing Forward, Moving Left
#define FRAME_BANK_RIGHT        16  // 16-17: Facing Forward, Moving Right

// RIGHT FACING
#define FRAME_RIGHT_IDLE        8   // 8-9
#define FRAME_TRANS_R_C         6   // 6-7: Turning Right->Center
#define FRAME_RIGHT_ACCEL       20  // 20-21: Facing Right, Moving Right (Forward)
#define FRAME_RIGHT_BRAKE       18  // 18-19: Facing Right, Moving Left (Backwards)

#define TURN_DURATION           15  // How many frames to hold before turning
#define MAX_SPEED              (1 * ONE_PIXEL + 8) // Max horizontal speed (in subpixels)
#define ACCEL_RATE      1       // 2/16ths pixel accel per frame
#define FRICTION_RATE   1       // 1/16th pixel friction

#define GRAVITY_SPEED   (1 << SUBPIXEL_BITS)   // Pixels per frame to fall when idle
#define CLIMB_SPEED     (2 << SUBPIXEL_BITS)   // Pixels per frame to rise
#define DIVE_SPEED      (2 << SUBPIXEL_BITS)   // Pixels per frame to force down

// Clouds
#define NUM_CLOUDS 3
#define MIN_CLOUD_Y     (10 << 4)  // 10 pixels from top
#define MAX_CLOUD_Y     (80 << 4)  // 80 pixels from top (don't hit mountains)


#endif // CONSTANTS_H