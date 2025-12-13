#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include "input.h"
#include "player.h"
#include "constants.h"


int16_t chopper_xl = SCREEN_WIDTH / 2;
int16_t chopper_xr = SCREEN_WIDTH / 2 + 16;
int16_t chopper_y = SCREEN_HEIGHT / 2;
int16_t chopper_frame = 0; // Current frame index (0-21)

void update_player(void)
{
    bool rotate_left = false;
    bool rotate_right = false;
    bool thrust = false;
    bool reverse_thrust = false;

    rotate_left = is_action_pressed(0, ACTION_ROTATE_LEFT);
    rotate_right = is_action_pressed(0, ACTION_ROTATE_RIGHT);
    thrust = is_action_pressed(0, ACTION_THRUST);
    reverse_thrust = is_action_pressed(0, ACTION_REVERSE_THRUST);

    // Update chopper position based on input
    if (thrust && !reverse_thrust) {
        // Move up
        chopper_y -= 1;
    } else if (reverse_thrust && !thrust) {
        // Move down
        chopper_y += 1;
    }
    if (rotate_left && !rotate_right) {
        // Move left
        chopper_xl -= 1;
        chopper_xr -= 1;
    } else if (rotate_right && !rotate_left) {
        // Move right
        chopper_xl += 1;
        chopper_xr += 1;
    }

}