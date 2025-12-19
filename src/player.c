#include <rp6502.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "input.h"
#include "player.h"
#include "constants.h"
#include "explosion.h"
#include "hostages.h"
#include "jet.h"
#include "sound.h"
#include "boom.h"


PlayerState player_state = PLAYER_ALIVE;
uint8_t death_timer = 0; // Generic timer for death animations

#define CHOPPER_START_POS_XL ((SCREEN_WIDTH / 2) - 16)  // Start roughly in middle of screen

// --- CAMERA & WORLD ---
// camera_x is the World Position of the left edge of the screen
int32_t chopper_world_x = (int32_t)CHOPPER_START_POS << SUBPIXEL_BITS;
int32_t camera_x = ((int32_t)CHOPPER_START_POS << SUBPIXEL_BITS) - (144 << SUBPIXEL_BITS);

// Positions are now stored as Sub-Pixels
int16_t chopper_xl = CHOPPER_START_POS_XL << SUBPIXEL_BITS;  // This tracks our on-screen X position
int16_t chopper_xr = (CHOPPER_START_POS_XL +16) << SUBPIXEL_BITS; // Right side is 16 pixels to the right
int16_t chopper_y = GROUND_Y_SUB; // (SCREEN_HEIGHT / 2) << SUBPIXEL_BITS;

int16_t chopper_frame = 0; // Current frame index (0-21)

// Global State Variables
ChopperHeading current_heading = FACING_LEFT;
int8_t  turn_timer = 0;       // Counts how long we hold a direction to turn
int16_t velocity_x = 0;
uint8_t blade_frame = 0;      // 0 or 1 (Animation toggle)
uint8_t anim_clock = 0;       // Timer for blade speed
bool is_turning = false;     // Are we currently rotating?
int8_t next_heading = 0;     // Where are we trying to go?

// Track the button state to prevent "machine gun" toggling when holding it
bool btn2_last_state = false;

// Track where we came from so we know which way to turn next
// -1 = Came from Left, 1 = Came from Right
int8_t last_side_facing = -1;

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

extern bool is_demo_mode;


void update_chopper_animation(uint8_t frame)
{
    // Update the chopper sprite to the specified frame
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_chopper_sprite_ptr(frame, 0));
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_chopper_sprite_ptr(frame, 1));
}

extern uint8_t anim_timer;

uint8_t base_frame = FRAME_CENTER_IDLE;
bool is_landed = true;

void respawn_player(void) {
    player_state = PLAYER_ALIVE;

    // Ensure Effects are hidden
    xram0_struct_set(BOOM_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
    
    // Reset Position (Home Base)
    chopper_world_x = (int32_t)CHOPPER_START_POS << SUBPIXEL_BITS;
    chopper_y = GROUND_Y_SUB;
    velocity_x = 0;
    
    // Reset Camera (Centered on Home)
    camera_x = chopper_world_x - (144 << SUBPIXEL_BITS);
    if (camera_x > CAMERA_MAX_X) camera_x = CAMERA_MAX_X; // Safety clamp  <-- Check for CAMERA_MAX_X definition

    // Reset Heading
    current_heading = FACING_LEFT; // Face the enemy
    
    // Reset any other flags
    is_turning = false;

    // --- NEW: CLEAR ENEMIES ---
    reset_jet();

    // Restart Engine Idle Sound immediately
    update_chopper_sound(0);
    
    // Note: We do NOT reset hostages_rescued or destroyed bases.
    // The war continues!
}

void kill_player(void) {
    if (player_state != PLAYER_ALIVE) return;

    player_state = PLAYER_DYING_FALLING;
    death_timer = 0;
    
    // --- SHOW HIT EFFECT (FRAME 0) ---
    int16_t screen_x = ((chopper_world_x - camera_x) >> SUBPIXEL_BITS) + 8;
    int16_t screen_y = (chopper_y >> SUBPIXEL_BITS) + 4;

    // Use shared function
    trigger_boom(screen_x, screen_y);

    sfx_explosion_large(); // Play explosion sound
}

void update_chopper_state(void) {
    
    // -----------------------------------------------------------
    // 1. GLOBAL ANIMATION TIMERS
    // -----------------------------------------------------------
    anim_timer++;
    if (anim_timer > 2) { 
        anim_timer = 0;
        blade_frame = !blade_frame;
    }

    // =========================================================
    // STATE: ALIVE (Your Existing Logic)
    // =========================================================
    if (player_state == PLAYER_ALIVE) {

        bool input_left = false;
        bool input_right = false;
        bool input_up = false;
        bool input_down = false;
        bool input_btn2 = false;

        static int8_t demo_direction = -1; // -1 = Left, 1 = Right
        static uint8_t demo_hover_cooldown = 0;

        if (is_demo_mode) {
            // ---------------------------------------------------------
        // 1. HORIZONTAL AI (Patrol World)
        // ---------------------------------------------------------
        // Define "Turnaround Points" slightly inside the world boundaries
        // so we don't hit the hard clamp.
        int32_t turn_left_x  = WORLD_MIN_X_SUB + (200 << SUBPIXEL_BITS);
        int32_t turn_right_x = WORLD_MAX_X_SUB - (200 << SUBPIXEL_BITS);

        // Check Position
        if (chopper_world_x <= turn_left_x) {
            demo_direction = 1; // Fly Right
        }
        else if (chopper_world_x >= turn_right_x) {
            demo_direction = -1; // Fly Left
        }

        // Apply Input
        if (demo_direction == -1) input_left = true;
        else input_right = true;

        // ---------------------------------------------------------
        // 2. VERTICAL AI (Bobbing / Hysteresis)
        // ---------------------------------------------------------
        // Target Altitude: 60 pixels from top
        int16_t target_alt = (60 << SUBPIXEL_BITS);

        if (demo_hover_cooldown > 0) {
            // Coasting / Falling Phase
            demo_hover_cooldown--;
            // No input_up -> Gravity takes over
        } 
        else {
            // Climbing Phase
            if (chopper_y > target_alt) {
                input_up = true; // Apply Thrust
            } 
            else {
                // We reached the top! Cut engines.
                // Set timer to ~45 frames (0.75 seconds) to let it drift down
                demo_hover_cooldown = 45; 
            }
        }
        } 
        else {
            // --- REAL PLAYER INPUT ---
            input_left = is_action_pressed(0, ACTION_ROTATE_LEFT);
            input_right = is_action_pressed(0, ACTION_ROTATE_RIGHT);
            input_up = is_action_pressed(0, ACTION_THRUST);
            input_down = is_action_pressed(0, ACTION_REVERSE_THRUST);
            input_btn2 = is_action_pressed(0, ACTION_SUPER_FIRE);
        }
        
        // --- TURN LOGIC (Button 2) ---
        if (input_btn2 && !btn2_last_state && !is_turning && !is_landed) {
            if (current_heading == FACING_LEFT) {
                next_heading = FACING_CENTER;
                last_side_facing = -1;
            }
            else if (current_heading == FACING_RIGHT) {
                next_heading = FACING_CENTER;
                last_side_facing = 1;
            }
            else { 
                if (last_side_facing == -1) next_heading = FACING_RIGHT;
                else next_heading = FACING_LEFT;
            }
            is_turning = true;
            turn_timer = TURN_DURATION;
        }
        btn2_last_state = input_btn2;

        // Helper: Are we currently touching the ground?
        is_landed = (chopper_y >= GROUND_Y_SUB);

        // --- PHYSICS & FRAME SELECTION ---
        if (is_turning) {
            turn_timer--;
            
            if (current_heading == FACING_LEFT || next_heading == FACING_LEFT) {
                base_frame = FRAME_TRANS_L_C;
            } else {
                base_frame = FRAME_TRANS_R_C;
            }

            // Friction
            if (velocity_x > 0) velocity_x -= FRICTION_RATE;
            if (velocity_x < 0) velocity_x += FRICTION_RATE;

            if (turn_timer == 0) {
                current_heading = next_heading;
                is_turning = false;
            }
        } 
        else {
            // NORMAL FLIGHT LOGIC
            if (current_heading == FACING_CENTER) {
                if (input_left && !is_landed) {
                    base_frame = FRAME_BANK_LEFT;
                    if (velocity_x > -ONE_PIXEL) velocity_x -= ACCEL_RATE; 
                } 
                else if (input_right && !is_landed) {
                    base_frame = FRAME_BANK_RIGHT;
                    if (velocity_x < ONE_PIXEL) velocity_x += ACCEL_RATE;
                } 
                else {
                    base_frame = FRAME_CENTER_IDLE;
                    if (velocity_x < 0) velocity_x += FRICTION_RATE;
                    if (velocity_x > 0) velocity_x -= FRICTION_RATE;
                }
            }
            else if (current_heading == FACING_LEFT) {
                if (input_left && !is_landed) {
                    base_frame = FRAME_LEFT_ACCEL;
                    if (velocity_x > -MAX_SPEED) velocity_x -= ACCEL_RATE;
                }
                else if (input_right && !is_landed) {
                    base_frame = FRAME_LEFT_BRAKE;
                    if (velocity_x < MAX_SPEED) velocity_x += ACCEL_RATE;
                }
                else {
                    base_frame = FRAME_LEFT_IDLE;
                    if (velocity_x < 0) velocity_x += FRICTION_RATE;
                    if (velocity_x > 0) velocity_x -= FRICTION_RATE;
                }
            }
            else if (current_heading == FACING_RIGHT) {
                if (input_right && !is_landed) {
                    base_frame = FRAME_RIGHT_ACCEL;
                    if (velocity_x < MAX_SPEED) velocity_x += ACCEL_RATE;
                }
                else if (input_left && !is_landed) {
                    base_frame = FRAME_RIGHT_BRAKE;
                    if (velocity_x > -MAX_SPEED) velocity_x -= ACCEL_RATE;
                }
                else {
                    base_frame = FRAME_RIGHT_IDLE;
                    if (velocity_x < 0) velocity_x += FRICTION_RATE;
                    if (velocity_x > 0) velocity_x -= FRICTION_RATE;
                }
            }
        }

        // --- VERTICAL PHYSICS ---
        if (input_up) {
            chopper_y -= CLIMB_SPEED;
        } 
        else if (input_down) {
            chopper_y += DIVE_SPEED;
        } 
        else {
            if (chopper_y < GROUND_Y_SUB) {
                chopper_y += GRAVITY_SPEED;
            }
        }

        // --- BOUNDARY CHECKS ---
        if (chopper_y < CEILING_Y_SUB) chopper_y = CEILING_Y_SUB;
        
        if (chopper_y > GROUND_Y_SUB) {
            chopper_y = GROUND_Y_SUB;
            velocity_x = 0; 
        }

        // Calculate next potential position
        int32_t next_world_x = chopper_world_x + velocity_x;
        int16_t next_pixel_x = next_world_x >> SUBPIXEL_BITS;

        if (next_pixel_x < WORLD_SIZE_MIN) {
            velocity_x = 0; 
            chopper_world_x = ((int32_t)WORLD_SIZE_MIN << SUBPIXEL_BITS);
        }
        else if (next_pixel_x > WORLD_SIZE_MAX) {
            velocity_x = 0;
            chopper_world_x = ((int32_t)WORLD_SIZE_MAX << SUBPIXEL_BITS);
        }
        else {
            chopper_world_x += velocity_x;
        }

        // Keep local trackers in sync (for compatibility)
        chopper_xl += velocity_x;
        chopper_xr += velocity_x;


        int16_t abs_vx = (velocity_x < 0) ? -velocity_x : velocity_x;
        int16_t abs_vy = 0;
        
        if (input_up) abs_vy = CLIMB_SPEED;
        else if (input_down) abs_vy = DIVE_SPEED;
        
        // Pass total speed (scaled down to fit 16-bit comfortably if needed)
        // velocity_x is subpixels (e.g. 32).
        update_chopper_sound(abs_vx + abs_vy);
        

    }

    // =========================================================
    // STATE: FALLING (Spinning Down)
    // =========================================================
    else if (player_state == PLAYER_DYING_FALLING) {

        stop_chopper_sound();
        
        // 1. Gravity (Fall Fast)
        chopper_y += (3 << SUBPIXEL_BITS); 
        
        // 2. Momentum (Maintain some forward speed)
        chopper_world_x += velocity_x;

        // 3. Spin Animation
        death_timer++;
        // Toggle frames every 4 ticks
        if ((death_timer / 4) % 2 == 0) base_frame = 12; // Nose Up Left
        else base_frame = 18; // Nose Up Right
        
        // 4. Boom Animation (Flash Effect)
        // if (death_timer == 5) {
        //     // Switch to Boom Frame 1
        //     xram0_struct_set(BOOM_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (uint16_t)(BOOM_DATA + 512));
        // }
        // else if (death_timer == 10) {
        //     // Hide Boom
        //     xram0_struct_set(BOOM_CONFIG, vga_mode4_sprite_t, y_pos_px, -32);
        // }

        // 5. Ground Impact
        if (chopper_y >= GROUND_Y_SUB) {
            chopper_y = GROUND_Y_SUB;
            
            // Trigger Crash Event
            player_state = PLAYER_DYING_CRASHING;
            death_timer = 0;
            
            // Big Explosion
            trigger_explosion(chopper_world_x + (8 << SUBPIXEL_BITS), GROUND_Y_SUB);
            
            // Kill everyone
            kill_all_passengers();
        }
    }

    // =========================================================
    // STATE: CRASHING (Exploding)
    // =========================================================
    else if (player_state == PLAYER_DYING_CRASHING) {
        death_timer++;
        if (death_timer > 60) {
            // respawn_player();
            player_state = PLAYER_WAITING_FOR_RESPAWN;
        }
    }

    // -----------------------------------------------------------
    // CAMERA UPDATE (Only if not fully crashed)
    // -----------------------------------------------------------
    if (player_state != PLAYER_DYING_CRASHING) {
        int16_t screen_rel_px = (chopper_world_x - camera_x) >> SUBPIXEL_BITS;

        if (screen_rel_px > SCROLL_RIGHT_EDGE) {
            camera_x += (chopper_world_x - camera_x) - (SCROLL_RIGHT_EDGE << SUBPIXEL_BITS);
        } else if (screen_rel_px < SCROLL_LEFT_EDGE) {
            camera_x += (chopper_world_x - camera_x) - (SCROLL_LEFT_EDGE << SUBPIXEL_BITS);
        }

        if (camera_x < 0) camera_x = 0;
        if (camera_x > CAMERA_MAX_X) camera_x = CAMERA_MAX_X;
    }

    // -----------------------------------------------------------
    // HARDWARE UPDATE
    // -----------------------------------------------------------
    
    // Calculate final screen positions
    int16_t hardware_xl = (chopper_world_x - camera_x) >> SUBPIXEL_BITS;
    int16_t hardware_xr = (chopper_world_x - camera_x + 256) >> SUBPIXEL_BITS; // +16px
    int16_t hardware_y = chopper_y >> SUBPIXEL_BITS;

    // If crashing, hide the chopper sprite so we only see the explosion
    if (player_state == PLAYER_DYING_CRASHING) {
        hardware_y = -32;
    }

    int final_frame_idx = base_frame + blade_frame;
    uint16_t ptr_offset = final_frame_idx * 1024; 

    // Left Half
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (uint16_t)(CHOPPER_DATA + ptr_offset));
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, x_pos_px, hardware_xl);
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, hardware_y);

    // Right Half
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (uint16_t)(CHOPPER_DATA + ptr_offset + 512));
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, x_pos_px, hardware_xr);
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, hardware_y);

    // Scroll
    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, x_pos_px, -((camera_x/2) >> SUBPIXEL_BITS));
}
