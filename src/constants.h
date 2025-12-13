#ifndef CONSTANTS_H
#define CONSTANTS_H

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// XRAM Memory Layout for RPMegaChopper
// ---------------------------------------------------------------------------
// We have a total of 65536 bytes (64KB) of XRAM to work with. We'll allocate
// space for the Chopper sprites, background tiles, maps, and soldier sprites.

// Chopper Sprite Sheet (22 frames, 2x 16x16 sprites per frame)
// Total Size: 22,528 bytes (0x5800)
#define SPRITE_DATA_START       0x0000 // Starting address in XRAM for sprite data
#define CHOPPER_DATA_SIZE       0x5800 // Size of the chopper sprite data
#define CHOPPER_DATA            SPRITE_DATA_START // Chopper sprite data address
#define SKY_DATA               (SPRITE_DATA_START + CHOPPER_DATA_SIZE) // Sky background data address
#define SKY_DATA_SIZE           0x0080 // Size of the sky background data

#define SPRITE_DATA_END        (SKY_DATA + SKY_DATA_SIZE) // End of used data area

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


// 4. TILE MAP CONFIGURATION
// -------------------------------------------------------------------------
// Configuration data for the sky background
extern unsigned SKY_MAP_START; // Sky Background Configuration
#define SKY_MAP_SIZE            0x0258  // 600 bytes
extern unsigned SKY_MAP_END;

// 3. REMAINING SPACE
// -------------------------------------------------------------------------
// Total XRAM: 65536
// Used:       22528
// Remaining:  43008 bytes (~42KB) for Tiles, Maps, and Soldier sprites!

#endif // CONSTANTS_H