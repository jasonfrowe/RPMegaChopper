#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include "player.h"
#include "clouds.h"
#include "constants.h"


int32_t cloud_world_x[NUM_CLOUDS] = { 100<<4, 300<<4, 500<<4 }; // Spread them out
int16_t cloud_y[NUM_CLOUDS] = { 30<<4, 50<<4, 20<<4 };          // Different heights
uint8_t cloud_depth_shift[NUM_CLOUDS]; // 1=Fast, 2=Med, 3=Slow

// --- CLOUD DRIFT CONFIG ---
// How often each cloud's world_x updates (in frames)
const uint8_t CLOUD_DRIFT_FREQUENCY[NUM_CLOUDS] = {
    1,   // Cloud 0: Updates every 1 frame (Fastest)
    4,   // Cloud 1: Updates every 4 frames (Medium, as before)
    16   // Cloud 2: Updates every 16 frames (Slower)
};

void update_clouds(void) {
    // Static timer persists between function calls. Increments every frame.
    static uint8_t drift_frame_counter = 0;
    drift_frame_counter++;

    for (int i = 0; i < NUM_CLOUDS; i++) {
        
        // --- SLOW DRIFT LOGIC (Rightward Motion) ---
        
        // Check if it's time to update this cloud's position
        if ((drift_frame_counter % CLOUD_DRIFT_FREQUENCY[i]) == 0) {
            cloud_world_x[i] += 1; // Move RIGHT (+) by 1 subpixel
        }

        // --- PARALLAX MATH ---
        int32_t cloud_screen_sub = cloud_world_x[i] - (camera_x >> cloud_depth_shift[i]);
        int16_t cloud_screen_px = cloud_screen_sub >> SUBPIXEL_BITS;

        // --- WRAPPING LOGIC ---
        if (cloud_screen_px < -32) {
            cloud_world_x[i] += (400 << SUBPIXEL_BITS); 
        }
        else if (cloud_screen_px > 350) { // Changed to 'else if'
            cloud_world_x[i] -= (400 << SUBPIXEL_BITS);
        }

        // --- WRITE TO HARDWARE ---
        unsigned cfg_addr = CLOUD_A_CONFIG + (i * sizeof(vga_mode4_sprite_t));
        xram0_struct_set(cfg_addr, vga_mode4_sprite_t, x_pos_px, cloud_screen_px);
        xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, cloud_y[i] >> SUBPIXEL_BITS);
    }
}