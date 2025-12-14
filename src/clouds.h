#ifndef CLOUDS_H
#define CLOUDS_H

// Clouds
#define NUM_CLOUDS 3
#define MIN_CLOUD_Y     (40 << 4)  // 10 pixels from top
#define MAX_CLOUD_Y     (80 << 4)  // 80 pixels from top (don't hit mountains)

extern int32_t cloud_world_x[];
extern int16_t cloud_y[];
extern uint8_t cloud_depth_shift[];

extern void update_clouds(void);

#endif // CLOUDS_H