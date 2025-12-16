#ifndef HOSTAGES_H
#define HOSTAGES_H

// Hostages
#define NUM_HOSTAGES    16
#define HOSTAGES_PER_BASE 16
#define HOSTAGE_RUN_SPEED 8 // (1 << SUBPIXEL_BITS) // 1 pixel per frame

#define SIGHT_RANGE (150 << SUBPIXEL_BITS)

typedef struct {
    bool destroyed;
    uint8_t hostages_remaining;
    uint8_t spawn_timer;
    uint8_t tanks_remaining; // NEW: How many tanks are inside?
} EnemyBase;

extern EnemyBase base_state[];

// --- HOSTAGE STATE DEFINITIONS ---
typedef enum {
    H_STATE_INACTIVE = 0,
    H_STATE_WAITING,        // Waiting inside enemy base
    H_STATE_RUNNING_CHOPPER,// Running towards player
    H_STATE_BOARDING,       // Entering the door
    H_STATE_ON_BOARD,       // Safe inside chopper
    H_STATE_RUNNING_HOME,   // Running to Home Base
    H_STATE_WAVING,         // Victory wave
    H_STATE_SAFE,           // Inside Home Base (Score)
    H_STATE_DYING           // Crushed/Shot
} HostageState;

typedef struct {
    HostageState state;     // Replaces simple 'active' bool logic
    int32_t world_x;
    int16_t y;
    int8_t direction;       // 1 = Right, -1 = Left
    uint8_t anim_frame;     
    uint8_t anim_timer;     
    uint8_t base_id;        
} Hostage;

extern Hostage hostages[];

extern uint8_t hostages_on_board;
extern uint8_t hostages_rescued_count;
extern uint8_t hostages_lost_count;

extern void update_hostages(void);

#endif // HOSTAGES_H