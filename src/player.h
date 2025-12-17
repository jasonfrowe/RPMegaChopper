#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

// ============================================================================
// PLAYER MODULE
// ============================================================================

#define LIVES_STARTING     3

#define FRAME_SPIN_LEFT  12
#define FRAME_SPIN_RIGHT 18

extern int16_t chopper_xl;
extern int16_t chopper_xr;
extern int16_t chopper_y;
extern int16_t chopper_frame;

extern void update_player(void);
extern uint16_t get_chopper_sprite_ptr(int frame_idx, int part);
extern void update_chopper_animation(uint8_t frame);
extern void update_chopper_state(void);
extern void kill_player(void);
extern void respawn_player(void);

extern uint8_t base_frame;
extern bool is_landed;

extern int32_t camera_x;
extern int32_t chopper_world_x;

typedef enum {
    FACING_LEFT = -1,
    FACING_CENTER = 0,
    FACING_RIGHT = 1
} ChopperHeading;

typedef enum {
    PLAYER_ALIVE,
    PLAYER_DYING_FALLING, // Spinning down
    PLAYER_DYING_CRASHING, // Exploding on ground
    PLAYER_WAITING_FOR_RESPAWN // New State
} PlayerState;

extern int lives; // Global lives counter

extern PlayerState player_state;
extern uint8_t death_timer;

extern ChopperHeading current_heading;

#endif // PLAYER_H
