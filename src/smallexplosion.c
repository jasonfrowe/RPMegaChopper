#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "constants.h"
#include "smallexplosion.h"
#include "player.h"

typedef struct {
    bool active;
    int32_t world_x;
    int16_t y; // Subpixels
    uint8_t frame;
    uint8_t timer;
} SmallExplosion;

SmallExplosion small_explosions[MAX_EXPLOSIONS];

// Helper to get offset. 8x8 sprite = 64 pixels * 2 bytes = 128 bytes per frame.
uint16_t get_small_exp_ptr(uint8_t frame) {
    return SMALL_EXPLOSION_DATA + (frame * 128); 
}

void spawn_small_explosion(int32_t wx, int16_t wy) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!small_explosions[i].active) {
            small_explosions[i].active = true;
            // Center the 8x8 explosion on the impact point
            // Impact is usually center of bullet. 
            // Explosion 8x8 -> offset by -4 pixels.
            small_explosions[i].world_x = wx - (4 << SUBPIXEL_BITS);
            small_explosions[i].y = wy - (4 << SUBPIXEL_BITS);
            
            small_explosions[i].frame = 0;
            small_explosions[i].timer = 0;
            break; // Spawned one, stop looking
        }
    }
}

void update_small_explosions(void) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        unsigned cfg_addr = SMALL_EXPLOSION_CONFIG + (i * sizeof(vga_mode4_sprite_t));

        if (!small_explosions[i].active) {
            // Ensure hidden
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, -8);
            continue;
        }

        // --- ANIMATION ---
        small_explosions[i].timer++;
        if (small_explosions[i].timer > SMALL_EXP_DELAY) {
            small_explosions[i].timer = 0;
            small_explosions[i].frame++;
            
            // Animation finished?
            if (small_explosions[i].frame >= SMALL_EXP_FRAMES) {
                small_explosions[i].active = false;
                xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, -8);
                continue;
            }
        }

        // --- RENDER ---
        int32_t screen_sub = small_explosions[i].world_x - camera_x;
        int16_t screen_px  = screen_sub >> SUBPIXEL_BITS;

        // Visibility Check (8x8 sprite)
        if (screen_px > -8 && screen_px < 328) {
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, x_pos_px, screen_px);
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, small_explosions[i].y >> SUBPIXEL_BITS);
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, xram_sprite_ptr, get_small_exp_ptr(small_explosions[i].frame));
        } else {
            // Visible logic is active, but physically off-screen
            xram0_struct_set(cfg_addr, vga_mode4_sprite_t, y_pos_px, -8);
        }
    }
}