#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include "player.h"
#include "clouds.h"
#include "constants.h"


int32_t cloud_world_x[NUM_CLOUDS] = { 100<<4, 300<<4, 500<<4 }; // Spread them out
int16_t cloud_y[NUM_CLOUDS] = { 30<<4, 50<<4, 20<<4 };          // Different heights
uint8_t cloud_depth_shift[NUM_CLOUDS]; // 1=Fast, 2=Med, 3=Slow

void update_clouds(void) {

    // -----------------------------------------------------------
    // UPDATE CLOUDS (Parallax)
    // -----------------------------------------------------------

    // for (int i = 0; i < NUM_CLOUDS; i++) {
        
    //     // 1. Current Shift Amount for this cloud
    //     uint8_t shift = cloud_depth_shift[i];

    //     // 2. Calculate Screen Position
    //     // Formula: Screen = World - (Camera / SpeedFactor)
    //     // We use >> shift to simulate distance.
    //     int32_t cloud_screen_sub = cloud_world_x[i] - (camera_x >> shift);
    //     int16_t cloud_screen_px = cloud_screen_sub >> SUBPIXEL_BITS;

    //     // 3. WRAPPING LOGIC
        
    //     // --- CASE A: Cloud went off LEFT edge (< -32) ---
    //     // It needs to reappear on the RIGHT side (~340px)
    //     if (cloud_screen_px < -32) {
            
    //         // Randomize Properties
    //         cloud_y[i] = (rand() % (MAX_CLOUD_Y - MIN_CLOUD_Y)) + MIN_CLOUD_Y;
    //         cloud_depth_shift[i] = (rand() % 3) + 1; // Pick new speed (1-3)
    //         shift = cloud_depth_shift[i];            // Update local var

    //         // Re-Anchor World Position
    //         // We want 'Screen X' to be 340 (Just off right edge)
    //         // World = Screen + (Camera >> Shift)
    //         int32_t target_screen_x = 340 << SUBPIXEL_BITS;
    //         cloud_world_x[i] = target_screen_x + (camera_x >> shift);
    //     }

    //     // --- CASE B: Cloud went off RIGHT edge (> 350) ---
    //     // (Happens when flying Left). Needs to reappear on LEFT side (-30px)
    //     else if (cloud_screen_px > 350) {
            
    //         // Randomize Properties
    //         cloud_y[i] = (rand() % (MAX_CLOUD_Y - MIN_CLOUD_Y)) + MIN_CLOUD_Y;
    //         cloud_depth_shift[i] = (rand() % 3) + 1;
    //         shift = cloud_depth_shift[i];

    //         // Re-Anchor World Position
    //         // We want 'Screen X' to be -30 (Just off left edge)
    //         int32_t target_screen_x = -(30 << SUBPIXEL_BITS);
    //         cloud_world_x[i] = target_screen_x + (camera_x >> shift);
    //     }

    //     // 4. WRITE TO HARDWARE
    //     unsigned cfg_addr = CLOUD_A_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        
    //     // Update Position
    //     xram0_struct_set(cfg_addr, vga_mode4_sprite_t, x_pos_px, cloud_screen_px);
    //     xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, cloud_y[i] >> SUBPIXEL_BITS);
        
    //     // Optional: Update Sprite Pointer if you have different Cloud Designs
    //     // int cloud_type = rand() % 2; // If you have cloud_type stored
    //     // xram0_struct_set(cfg_addr, vga_mode4_sprite_t, xram_sprite_ptr, CLOUD_DATA_START + (cloud_type * 512));
    // }

    for (int i = 0; i < NUM_CLOUDS; i++) {
        // PARALLAX MATH:
        // Cloud Screen X = Cloud World X - (Camera X * 0.5)
        // We use >> 1 to divide camera position by 2.
        int32_t cloud_screen_sub = cloud_world_x[i] - (camera_x >> cloud_depth_shift[i]);
        int16_t cloud_screen_px = cloud_screen_sub >> SUBPIXEL_BITS;

        // WRAPPING LOGIC (Infinite Clouds)
        // If cloud goes off the Left edge, teleport to Right
        if (cloud_screen_px < -32) {
            // Add 'Screen Width' in World Units to the cloud
            // 320 pixels * 2 (because of 0.5 parallax) = 640 world pixels
            cloud_world_x[i] += (400 << SUBPIXEL_BITS); 
        }
        // If cloud goes off Right edge (when flying left)
        if (cloud_screen_px > 350) {
            cloud_world_x[i] -= (400 << SUBPIXEL_BITS);
        }

        // WRITE TO HARDWARE
        unsigned cfg_addr = CLOUD_A_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        xram0_struct_set(cfg_addr, vga_mode4_sprite_t, x_pos_px, cloud_screen_px);
        
        // Y position (Fixed height)
        xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, cloud_y[i] >> SUBPIXEL_BITS);
    }

}