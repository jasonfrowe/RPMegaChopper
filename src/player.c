#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include "input.h"
#include "player.h"
#include "constants.h"

int16_t chopper_xl = SCREEN_WIDTH / 2;
int16_t chopper_xr = SCREEN_WIDTH / 2 + 16;
int16_t chopper_y = SCREEN_HEIGHT / 2;
int16_t chopper_frame = 0; // Current frame index (0-21)

typedef enum {
    FACING_LEFT = -1,
    FACING_CENTER = 0,
    FACING_RIGHT = 1
} ChopperHeading;

// Global State Variables
ChopperHeading current_heading = FACING_CENTER;
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


void update_chopper_animation(uint8_t frame)
{
    // Update the chopper sprite to the specified frame
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_chopper_sprite_ptr(frame, 0));
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, get_chopper_sprite_ptr(frame, 1));
}

extern uint8_t anim_timer;

void update_chopper_state(void) {
    uint8_t base_frame = FRAME_CENTER_IDLE;
    
    // -----------------------------------------------------------
    // 1. UPDATE BLADE ANIMATION (0 or 1)
    // -----------------------------------------------------------
    anim_timer++;
    if (anim_timer > 2) { // Change frame every 2 VBlanks
        anim_timer = 0;
        blade_frame = !blade_frame;
    }

    // -----------------------------------------------------------
    // 2. DETERMINE FRAME BASED ON HEADING & INPUT
    // -----------------------------------------------------------

    bool input_left = false;
    bool input_right = false;
    bool input_up = false;
    bool input_down = false;
    bool input_btn2 = false;

    input_left = is_action_pressed(0, ACTION_ROTATE_LEFT);
    input_right = is_action_pressed(0, ACTION_ROTATE_RIGHT);
    input_up = is_action_pressed(0, ACTION_THRUST);
    input_down = is_action_pressed(0, ACTION_REVERSE_THRUST);
    input_btn2 = is_action_pressed(0, ACTION_SUPER_FIRE);
    
    // -----------------------------------------------------------
    // 2. INPUT HANDLING (BUTTON 2)
    // -----------------------------------------------------------
    // Only accept input if we aren't already turning
    if (input_btn2 && !btn2_last_state && !is_turning) {
        
        // Calculate the Target Heading
        if (current_heading == FACING_LEFT) {
            next_heading = FACING_CENTER;
            last_side_facing = -1;
        }
        else if (current_heading == FACING_RIGHT) {
            next_heading = FACING_CENTER;
            last_side_facing = 1;
        }
        else { 
            // At Center -> Go to the "other" side
            if (last_side_facing == -1) {
                next_heading = FACING_RIGHT;
            } else {
                next_heading = FACING_LEFT;
            }
        }

        // Start the Turn
        is_turning = true;
        turn_timer = TURN_DURATION;
    }
    btn2_last_state = input_btn2;

    // -----------------------------------------------------------
    // 3. PHYSICS & FRAME SELECTION
    // -----------------------------------------------------------

    if (is_turning) {
        // --- TURNING LOGIC ---
        turn_timer--;

        // Determine which transition frame to show
        // If turning Left<->Center, use Frame 2
        // If turning Right<->Center, use Frame 6
        if (current_heading == FACING_LEFT || next_heading == FACING_LEFT) {
            base_frame = FRAME_TRANS_L_C;
        } else {
            base_frame = FRAME_TRANS_R_C;
        }

        // Apply Friction while turning (Helicopter shouldn't stop instantly)
        if (velocity_x > 0) velocity_x--;
        if (velocity_x < 0) velocity_x++;

        // End of Turn
        if (turn_timer == 0) {
            current_heading = next_heading;
            is_turning = false;
        }
    } 
    else {
        // --- NORMAL FLIGHT LOGIC (Existing Code) ---
        
        // CASE A: FACING CENTER
        if (current_heading == FACING_CENTER) {
            if (input_left) {
                base_frame = FRAME_BANK_LEFT;
                if (velocity_x > -1) velocity_x--; 
            } 
            else if (input_right) {
                base_frame = FRAME_BANK_RIGHT;
                if (velocity_x < 1) velocity_x++;
            } 
            else {
                base_frame = FRAME_CENTER_IDLE;
                if (velocity_x < 0) velocity_x++; // Friction
                if (velocity_x > 0) velocity_x--;
            }
        }
        // CASE B: FACING LEFT
        else if (current_heading == FACING_LEFT) {
            if (input_left) {
                base_frame = FRAME_LEFT_ACCEL;
                if (velocity_x > -MAX_SPEED) velocity_x--;
            }
            else if (input_right) {
                base_frame = FRAME_LEFT_BRAKE;
                if (velocity_x < MAX_SPEED) velocity_x++;
            }
            else {
                base_frame = FRAME_LEFT_IDLE;
                if (velocity_x < 0) velocity_x++;
                if (velocity_x > 0) velocity_x--;
            }
        }
        // CASE C: FACING RIGHT
        else if (current_heading == FACING_RIGHT) {
            if (input_right) {
                base_frame = FRAME_RIGHT_ACCEL;
                if (velocity_x < MAX_SPEED) velocity_x++;
            }
            else if (input_left) {
                base_frame = FRAME_RIGHT_BRAKE;
                if (velocity_x > -MAX_SPEED) velocity_x--;
            }
            else {
                base_frame = FRAME_RIGHT_IDLE;
                if (velocity_x < 0) velocity_x++;
                if (velocity_x > 0) velocity_x--;
            }
        }
    }


    // -----------------------------------------------------------
    // 4. APPLY VERTICAL PHYSICS (GRAVITY)
    // -----------------------------------------------------------
    
    if (input_up) {
        // Active Climb (Fight Gravity)
        chopper_y -= CLIMB_SPEED;
    } 
    else if (input_down) {
        // Active Dive (With Gravity)
        chopper_y += DIVE_SPEED;
    } 
    else {
        // No Input -> Passive Descent (Gravity)
        // Check if we are already on the ground to prevent "jitter"
        if (chopper_y < GROUND_Y) {
            chopper_y += GRAVITY_SPEED;
        }
    }

    // -----------------------------------------------------------
    // 5. BOUNDARY CHECKS (CLAMP)
    // -----------------------------------------------------------

    // Hit the Ceiling?
    if (chopper_y < CEILING_Y) {
        chopper_y = CEILING_Y;
    }
    
    // Hit the Ground?
    if (chopper_y > GROUND_Y) {
        chopper_y = GROUND_Y;
        
        // Optional: If you want to kill horizontal momentum when landing
        velocity_x = 0; 
    }

    // Left Boundary
    if (chopper_xl < LEFT_BOUNDARY) {
        chopper_xl = LEFT_BOUNDARY;
        chopper_xr = chopper_xl + 16;
        // velocity_x = 0;
    }

    // Right Boundary
    if (chopper_xr > RIGHT_BOUNDARY) {
        chopper_xr = RIGHT_BOUNDARY;
        chopper_xl = chopper_xr - 16;
        // velocity_x = 0;
    }

    // -----------------------------------------------------------
    // 4. APPLY TO HARDWARE
    // -----------------------------------------------------------
    
    // Apply Horizontal Velocity
    chopper_xl += velocity_x;
    chopper_xr += velocity_x;

    // // Apply Vertical Movement (Standard layout: Up=Minus, Down=Plus)
    // if (input_up)   chopper_y -= 2;
    // if (input_down) chopper_y += 2;

    // Calculate Final Pointer (Base + Blade Toggle)
    // 1024 bytes per Frame Pair (Left Sprite + Right Sprite)
    int final_frame_idx = base_frame + blade_frame;
    uint16_t ptr_offset = final_frame_idx * 1024; 

    // Update XRAM Configs (Safe Addresses from previous step)
    
    // Left Half
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (uint16_t)(CHOPPER_DATA + ptr_offset));
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, x_pos_px, chopper_xl);
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, chopper_y);

    // Right Half (Offset by 512 bytes for the image data)
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (uint16_t)(CHOPPER_DATA + ptr_offset + 512));
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, x_pos_px, chopper_xr);
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, chopper_y);
}