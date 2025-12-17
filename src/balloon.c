#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "constants.h"
#include "player.h"
#include "input.h" // For globals like camera_x
#include "balloon.h"
#include "hostages.h"
#include "enemybase.h"
#include "explosion.h"
#include "homebase.h"

Balloon balloon;

// Sprite Data Offsets (16x16 = 512 bytes)
// Frame 0: Bottom=0, Top=512
// Frame 1: Bottom=1024, Top=1536
// Frame 2: Bottom=2048, Top=2560
uint16_t get_balloon_ptr(int frame, int part) {
    // part 0 = Bottom, part 1 = Top
    int base_idx = frame * 2; 
    return BALLOON_DATA + ((base_idx + part) * 512);
}

void update_balloon(void) {
    int total_progress = hostages_total_spawned;

    // =========================================================
    // 1. SPAWN / RESPAWN LOGIC
    // =========================================================
    if (!balloon.active) {
        if (balloon.respawn_timer > 0) {
            balloon.respawn_timer--;
            
            // HIDE SPRITES while waiting
            xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
            xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
            return;
        }

        // --- DETERMINE SPAWN ZONE ---
        int base_a = -1, base_b = -1;

        if (total_progress >= 48)      { base_a = 0; base_b = 1; }
        else if (total_progress >= 32) { base_a = 1; base_b = 2; }
        else if (total_progress >= 16) { base_a = 2; base_b = 3; }
        
        // If we are in a valid progression stage
        if (base_a != -1) {
            // Calculate Midpoint between the two bases
            int32_t x1 = ENEMY_BASE_LOCATIONS[base_a];
            int32_t x2 = ENEMY_BASE_LOCATIONS[base_b];
            int32_t spawn_x = (x1 + x2) / 2;

            // VISIBILITY CHECK
            // Only spawn if off-screen
            int32_t screen_sub = spawn_x - camera_x;
            int16_t screen_px = screen_sub >> SUBPIXEL_BITS;

            if (screen_px < -32 || screen_px > 350) {
                // SPAWN!
                balloon.active = true;
                balloon.world_x = spawn_x;
                balloon.y = GROUND_Y_SUB - (60 << SUBPIXEL_BITS); // Start high up
                balloon.vx = 0;
                balloon.vy = 0;
                balloon.anim_frame = 0;
            }
        }
        return; // Don't render if inactive
    }

    // =========================================================
    // 2. AI: HOMING
    // =========================================================
    
    // --- Define Boundaries ---
    // Safety Line: Halfway between the last enemy base (Index 3) and Home
    int32_t safety_line = (ENEMY_BASE_LOCATIONS[3] + HOMEBASE_WORLD_X) / 2;
    
    // Floor Ceiling: 32 pixels above ground
    int32_t min_altitude = GROUND_Y_SUB - (32 << SUBPIXEL_BITS);
    
    // Target: Chopper Center
    int32_t target_x = chopper_world_x + (16 << SUBPIXEL_BITS);
    int32_t target_y = chopper_y + (8 << SUBPIXEL_BITS);

    // --- X Movement ---
    // If balloon tries to go past the safety line towards home, force it back
    if (balloon.world_x > safety_line) {
        balloon.vx = -BALLOON_SPEED; // Go Left
    } 
    else {
        // Normal Homing
        if (balloon.world_x < target_x) balloon.vx = BALLOON_SPEED;
        else balloon.vx = -BALLOON_SPEED;
    }

    // --- Y Movement ---
    if (balloon.y < target_y) balloon.vy = BALLOON_SPEED;
    else balloon.vy = -BALLOON_SPEED;

    // --- Apply Physics ---
    balloon.world_x += balloon.vx;
    balloon.y += balloon.vy;

    // --- Clamp Altitude ---
    // If balloon is too low, force it up to the minimum altitude
    if (balloon.y > min_altitude) {
        balloon.y = min_altitude;
    }

    // =========================================================
    // 3. COLLISION: PLAYER (The Balloon hits the Chopper)
    // =========================================================
    if (player_state == PLAYER_ALIVE) {
        // Simple Box Collision
        if (labs(balloon.world_x - target_x) < (12 << SUBPIXEL_BITS)) { // Width
            if (labs(balloon.y - target_y) < (16 << SUBPIXEL_BITS)) {   // Height
                
                // CRASH PLAYER
                kill_player();
                
                // DESTROY BALLOON
                trigger_explosion(balloon.world_x, balloon.y); // Visual
                balloon.active = false;
                balloon.respawn_timer = BALLOON_RESPAWN;
                return;
            }
        }
    }

    // =========================================================
    // 4. RENDER
    // =========================================================
    
    // Animation (Slow bobbing)
    balloon.anim_timer++;
    if (balloon.anim_timer > 8) {
        balloon.anim_timer = 0;
        balloon.anim_frame++;
        if (balloon.anim_frame > 2) balloon.anim_frame = 0;
    }

    int32_t screen_sub = balloon.world_x - camera_x;
    int16_t screen_px = screen_sub >> SUBPIXEL_BITS;
    int16_t screen_y = balloon.y >> SUBPIXEL_BITS;

    if (screen_px > -16 && screen_px < 336) {
        
        // Draw Bottom
        xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_px);
        xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, y_pos_px, screen_y);
        xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_balloon_ptr(balloon.anim_frame, 0));

        // Draw Top (16px higher)
        xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, x_pos_px, screen_px);
        xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, y_pos_px, (screen_y - 16));
        xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_balloon_ptr(balloon.anim_frame, 1));
        
    } else {
        // Offscreen hide
        xram0_struct_set(BALLOON_BOTTOM_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        xram0_struct_set(BALLOON_TOP_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    }
}