#ifndef EBULLETS_H
#define EBULLETS_H

// Enemy Bullets
#define NEBULLET 2 // Number of enemy bullets   

// Physics Constants
#define TANK_BULLET_GRAVITY     2 // (1 << (SUBPIXEL_BITS - 2)) // 0.25 pixels/frame^2
#define TANK_BULLET_LAUNCH_VY   -(4 << SUBPIXEL_BITS)      // Initial upward burst
#define TANK_BULLET_SPEED_X     (2 << SUBPIXEL_BITS)       // Horizontal speed

extern void update_tank_bullets(void);

#endif // EBULLETS_H