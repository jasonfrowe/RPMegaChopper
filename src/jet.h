#ifndef JET_H
#define JET_H

// --- CONFIGURATION ---
#define JET_SPEED_X         (4 << SUBPIXEL_BITS) // Fast!
#define JET_CLIMB_SPEED     (2 << SUBPIXEL_BITS)
#define JET_BULLET_SPEED    (6 << SUBPIXEL_BITS)
#define JET_BOMB_GRAVITY    2 

// Triggers
#define JET_MIN_PROGRESS    24
#define TIMER_GROUND_MAX    120 // 2 Seconds
#define TIMER_AIR_MAX       240 // 4 Seconds
#define AIR_ZONE_Y          (75 << SUBPIXEL_BITS) // Top 75 pixels

extern void update_jet(void);
extern void reset_jet(void);

// --- STATE ---
typedef enum {
    JET_INACTIVE,
    JET_FLYING_ATTACK, // Approaching
    JET_ARCING_AWAY    // Leaving
} JetState;

typedef enum {
    WEAPON_NONE,
    WEAPON_BOMB,
    WEAPON_BULLET
} JetWeaponType;

typedef struct {
    JetState state;
    int32_t world_x;
    int32_t y;
    int8_t direction; // 1 = Right, -1 = Left
    
    // Weapon State
    JetWeaponType weapon_type;
    bool weapon_active;
    int32_t w_x, w_y;
    int16_t w_vx, w_vy;
    
    // Logic flags
    bool has_fired;
} FighterJet;

extern FighterJet jet;


#endif // JET_H