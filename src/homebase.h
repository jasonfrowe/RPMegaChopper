#ifndef HOMEBASE_H
#define HOMEBASE_H

// Home Base
#define NUM_HOMEBASE_SPRITE    6

// Place it at World X = 600 (Right after the landing pad)
#define HOMEBASE_WORLD_X    (3972L << SUBPIXEL_BITS)

// ============================================================================
// HOMEBASE MODULE
// ============================================================================
extern void update_homebase(void);

#endif // HOMEBASE_H