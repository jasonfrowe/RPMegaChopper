#ifndef SMALLEXPLOSION_H
#define SMALLEXPLOSION_H

// Small Explosion Animation
#define MAX_EXPLOSIONS 16
#define SMALL_EXP_FRAMES 7
#define SMALL_EXP_DELAY 3 // Ticks per frame (Animation Speed)

extern void spawn_small_explosion(int32_t wx, int16_t wy);
extern void update_small_explosions(void);

#endif // SMALLEXPLOSION_H