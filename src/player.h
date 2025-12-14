#ifndef PLAYER_H
#define PLAYER_H

// ============================================================================
// PLAYER MODULE
// ============================================================================
extern int16_t chopper_xl;
extern int16_t chopper_xr;
extern int16_t chopper_y;
extern int16_t chopper_frame;

extern void update_player(void);
extern uint16_t get_chopper_sprite_ptr(int frame_idx, int part);
extern void update_chopper_animation(uint8_t frame);
extern void update_chopper_state(void);

extern int32_t camera_x;

#endif // PLAYER_H
