#include "global.h"

/*
 * Can define globals here (or as macros in global.h).
 * not true: Don't include global.h (which has externs).
 */

/* Mapping from our standard order to their N_ order.
 */
short BUTTONS[8] = {
    N_LEFT, N_RIGHT,
    N_UP, N_DOWN,
    N_SELECT, N_START,
    N_B, N_A
};
