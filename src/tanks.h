#ifndef TANKS_H
#define TANKS_H

#define NUM_TANKS 2
#define SPRITES_PER_TANK 9 
// Body (5) + Turret (4) = 9

// --- TANK STATE ---
#define TANK_WIDTH_PX       40  // 5 * 8px
#define TANK_HEIGHT_PX      16  // Body(8) + Turret(8)
#define TANK_SPEED          (1 << SUBPIXEL_BITS) // Slow movement

typedef enum {
    TURRET_UP_LEFT = 0,
    TURRET_UP      = 1,
    TURRET_RIGHT   = 2
} TurretDir;

typedef struct {
    bool active;
    int32_t world_x;
    int16_t y;
    int8_t direction;       // 1 = Right, -1 = Left
    uint8_t anim_frame;     // 0 or 1 (Treads moving)
    uint8_t anim_timer;
    TurretDir turret_dir;   // Where is it aiming?
    int health;
} Tank;

extern Tank tanks[];
extern const int32_t TANK_SPAWNS[];

extern void update_tanks(void);

#endif // TANKS_H