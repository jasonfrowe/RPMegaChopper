#ifndef TANKS_H
#define TANKS_H

#define NUM_TANKS 2
#define SPRITES_PER_TANK 9 
// Body (5) + Turret (4) = 9

// --- TANK STATE ---
#define TANK_WIDTH_PX       40  // 5 * 8px
#define TANK_HEIGHT_PX      16  // Body(8) + Turret(8)
#define TANK_SPEED          (1 << SUBPIXEL_BITS) // Slow movement
    
// --- CONFIGURATION ---
#define TANKS_PER_BASE      2
#define TANK_LEASH_DIST     (100 << SUBPIXEL_BITS) // Don't drive more than 300px from base
#define TANK_SPEED          (1 << SUBPIXEL_BITS)   // 1 pixel per frame (Slow)
#define TANK_SPAWN_TRIGGER  4                      // Hostages required to trigger tanks
#define TANK_SPAWN_OFFSET_X (200L << SUBPIXEL_BITS) // Distance from base center to spawn tanks
#define SCREEN_WIDTH_SUB    (320L << SUBPIXEL_BITS)
#define OFFSCREEN_BUFFER    (32L << SUBPIXEL_BITS) // Spawn 32px off-screen
#define TANK_DESPAWN_DIST   (400L << SUBPIXEL_BITS) // Distance to recycle tank
#define TANK_SPACING        (45L << SUBPIXEL_BITS)  // Min distance between tanks
#define TANK_SPAWN_DIST     (320L << SUBPIXEL_BITS) // Tank spawn distance from enemy base
  


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
    uint8_t anim_frame;     
    uint8_t anim_timer;
    TurretDir turret_dir;   
    int health;
    uint8_t base_id;        // NEW: Which base does this tank belong to?
} Tank;

extern Tank tanks[];
extern const int32_t TANK_SPAWNS[];
extern bool tanks_triggered;

extern void update_tanks(void);

#endif // TANKS_H