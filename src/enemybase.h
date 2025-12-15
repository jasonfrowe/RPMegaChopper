#ifndef ENEMYBASE_H
#define ENEMYBASE_H

// Enemy Base
#define NUM_ENEMYBASE_SPRITE   2  // 2x 32x32 sprites (top and bottom)

#define NUM_ENEMY_BASES 4


extern int32_t ENEMY_BASE_LOCATIONS[];

extern void update_enemybase(void);

#endif // ENEMYBASE_H