#ifndef BOMB_H
#define BOMB_H

// Configuration
// 2 pixels per frame drop speed (accelerating looks better, but constant is easier for now)
#define BOMB_SPEED_Y      (3 << SUBPIXEL_BITS) 

// Altitude Threshold: Above this height (60px), you drop "Deep" bombs
#define BOMB_DEPTH_ALTITUDE (GROUND_Y_SUB - (60 << SUBPIXEL_BITS))

// Target Planes
#define TARGET_Y_GROUND   (GROUND_Y_SUB + (16 << SUBPIXEL_BITS))
#define TARGET_Y_TANKS    (GROUND_Y_SUB + (32 << SUBPIXEL_BITS))

extern void update_bomb(void);

#endif // BOMB_H