#if !defined(BALLOON_H)
#define BALLOON_H

// Configuration
#define BALLOON_SPEED       (1 << SUBPIXEL_BITS) // Slow, relentless
#define BALLOON_RESPAWN     300 // 5 seconds
#define BALLOON_WIDTH       (12 << SUBPIXEL_BITS) // Hitbox width
#define BALLOON_HEIGHT      (28 << SUBPIXEL_BITS) // Hitbox height

// --- BALLOON STATE ---
typedef struct {
    bool active;
    int32_t world_x;
    int32_t y;
    int16_t vx, vy;
    uint8_t anim_frame;
    uint8_t anim_timer;
    int16_t respawn_timer; // 5 seconds = 300 ticks
} Balloon;

extern Balloon balloon;

void update_balloon(void);
void reset_balloon(void);

#endif // BALLOON_H