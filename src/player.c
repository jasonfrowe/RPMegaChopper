#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "input.h"
#include "player.h"
#include "constants.h"

int32_t cloud_world_x[NUM_CLOUDS] = { 100<<4, 300<<4, 500<<4 }; // Spread them out
int16_t cloud_y[NUM_CLOUDS] = { 30<<4, 50<<4, 20<<4 };          // Different heights
uint8_t cloud_depth_shift[NUM_CLOUDS]; // 1=Fast, 2=Med, 3=Slow


// --- CAMERA & WORLD ---
// camera_x is the World Position of the left edge of the screen
int32_t camera_x = 0; 
int32_t chopper_world_x = 160 << SUBPIXEL_BITS; // Start in middle of world


// Positions are now stored as Sub-Pixels
// Start at 160 * 16 = 2560
int16_t chopper_xl = (SCREEN_WIDTH / 2) << SUBPIXEL_BITS;
int16_t chopper_xr = ((SCREEN_WIDTH / 2) + 16) << SUBPIXEL_BITS;
int16_t chopper_y = (SCREEN_HEIGHT / 2) << SUBPIXEL_BITS;

// int16_t chopper_xl = SCREEN_WIDTH / 2;
// int16_t chopper_xr = SCREEN_WIDTH / 2 + 16;
// int16_t chopper_y = SCREEN_HEIGHT / 2;
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

    // Helper: Are we currently touching the ground?
    bool is_landed = (chopper_y >= GROUND_Y_SUB);

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
        if (velocity_x > 0) velocity_x -= FRICTION_RATE;
        if (velocity_x < 0) velocity_x += FRICTION_RATE;

        // End of Turn
        if (turn_timer == 0) {
            current_heading = next_heading;
            is_turning = false;
        }
    } 
    else {
        // --- NORMAL FLIGHT LOGIC ---
        
        // CASE A: FACING CENTER
        if (current_heading == FACING_CENTER) {
            if (input_left && !is_landed) {
                base_frame = FRAME_BANK_LEFT;
                // Drift Left
                if (velocity_x > -ONE_PIXEL) velocity_x -= ACCEL_RATE; 
            } 
            else if (input_right && !is_landed) {
                base_frame = FRAME_BANK_RIGHT;
                // Drift Right
                if (velocity_x < ONE_PIXEL) velocity_x += ACCEL_RATE;
            } 
            else {
                base_frame = FRAME_CENTER_IDLE;
                // Sub-pixel Friction
                if (velocity_x < 0) velocity_x += FRICTION_RATE;
                if (velocity_x > 0) velocity_x -= FRICTION_RATE;
            }
        }
        // CASE B: FACING LEFT
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
        // CASE C: FACING RIGHT
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
        if (chopper_y < GROUND_Y_SUB) {
            chopper_y += GRAVITY_SPEED;
        }
    }

    // -----------------------------------------------------------
    // 5. BOUNDARY CHECKS (CLAMP)
    // -----------------------------------------------------------

    // Hit the Ceiling?
    if (chopper_y < CEILING_Y_SUB) {
        chopper_y = CEILING_Y_SUB;
    }
    
    // Hit the Ground?
    if (chopper_y > GROUND_Y_SUB) {
        chopper_y = GROUND_Y_SUB;
        
        // Optional: If you want to kill horizontal momentum when landing
        velocity_x = 0; 
    }

    // Left Boundary
    if (chopper_xl < LEFT_BOUNDARY_SUB) {
        chopper_xl = LEFT_BOUNDARY_SUB;
        chopper_xr = chopper_xl + 256; // if SUBPIXEL_BITS changed, update here too
        // velocity_x = 0;
    }

    // Right Boundary
    if (chopper_xr > RIGHT_BOUNDARY_SUB) {
        chopper_xr = RIGHT_BOUNDARY_SUB;
        chopper_xl = chopper_xr - 256; // if SUBPIXEL_BITS changed, update here too
        // velocity_x = 0;
    }

    // -----------------------------------------------------------
    // 6. APPLY TO POSITION
    // -----------------------------------------------------------
    
    // Apply Horizontal Velocity
    chopper_xl += velocity_x;
    chopper_xr += velocity_x;

    // Update World Position
    chopper_world_x += velocity_x;

    // Calculate Screen Position (Where is the chopper relative to camera?)
    int16_t screen_x_sub = chopper_world_x - camera_x;
    int16_t screen_x_px  = screen_x_sub >> SUBPIXEL_BITS;

    // Pushing Right Edge?
    if (screen_x_px > SCROLL_RIGHT_EDGE) {
        // Move camera right to keep chopper at edge
        int32_t diff = screen_x_sub - (SCROLL_RIGHT_EDGE << SUBPIXEL_BITS);
        camera_x += diff;
    }
    
    // Pushing Left Edge?
    if (screen_x_px < SCROLL_LEFT_EDGE) {
        // Move camera left
        int32_t diff = screen_x_sub - (SCROLL_LEFT_EDGE << SUBPIXEL_BITS);
        camera_x += diff;
    }

    // -----------------------------------------------------------
    // 7. UPDATE TO HARDWARE
    // -----------------------------------------------------------

    // We convert from Sub-Pixels to Screen Pixels here using shift (>>)
    // int16_t hardware_xl = chopper_xl >> SUBPIXEL_BITS;
    // int16_t hardware_xr = chopper_xr >> SUBPIXEL_BITS;
    // int16_t hardware_y = chopper_y >> SUBPIXEL_BITS;
    int16_t hardware_xl = (chopper_world_x - camera_x) >> SUBPIXEL_BITS;
    int16_t hardware_xr = (chopper_world_x - camera_x + 256) >> SUBPIXEL_BITS;
    int16_t hardware_y = chopper_y >> SUBPIXEL_BITS;

    // Calculate Final Pointer (Base + Blade Toggle)
    // 1024 bytes per Frame Pair (Left Sprite + Right Sprite)
    int final_frame_idx = base_frame + blade_frame;
    uint16_t ptr_offset = final_frame_idx * 1024; 

    // Update XRAM Configs (Safe Addresses from previous step)
    
    // Left Half
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (uint16_t)(CHOPPER_DATA + ptr_offset));
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, x_pos_px, hardware_xl);
    xram0_struct_set(CHOPPER_LEFT_CONFIG, vga_mode4_sprite_t, y_pos_px, hardware_y);

    // Right Half (Offset by 512 bytes for the image data)
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, (uint16_t)(CHOPPER_DATA + ptr_offset + 512));
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, x_pos_px, hardware_xr);
    xram0_struct_set(CHOPPER_RIGHT_CONFIG, vga_mode4_sprite_t, y_pos_px, hardware_y);


    xram0_struct_set(GROUND_CONFIG, vga_mode2_config_t, x_pos_px, -(camera_x >> SUBPIXEL_BITS));

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
        int32_t cloud_screen_sub = cloud_world_x[i] - (camera_x >> 1);
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