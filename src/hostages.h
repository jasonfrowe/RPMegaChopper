#ifndef HOSTAGES_H
#define HOSTAGES_H

// Hostages
#define NUM_HOSTAGES    16
#define HOSTAGES_PER_BASE 16
#define HOSTAGE_RUN_SPEED (1 << SUBPIXEL_BITS) // 1 pixel per frame

typedef struct {
    bool destroyed;
    uint8_t hostages_remaining;
    uint8_t spawn_timer;    // Delay between spawns
} EnemyBase;

extern EnemyBase base_state[];

typedef struct {
    bool active;
    int32_t world_x;
    int16_t y;
    int8_t direction;       // 1 = Right, -1 = Left, 0 = Idle
    uint8_t anim_frame;     // Current animation frame index (0-9)
    uint8_t anim_timer;     // Timer for animation speed
    uint8_t base_id;        // Which base did they come from?
} Hostage;

extern Hostage hostages[];

extern void update_hostages(void);

#endif // HOSTAGES_H