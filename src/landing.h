#ifndef LANDING_H
#define LANDING_H


// World X Position of the Landing Pad (in Subpixels)
#define LANDING_PAD_WORLD_X   (104 << SUBPIXEL_BITS)

// Landing Pad
#define NUM_LANDING_PAD_SPRITE 9

extern void update_landing(void);

#endif // LANDING_H