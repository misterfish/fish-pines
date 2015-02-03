#include "buttons.h"

#define new_rule(mask, kill_multiple, func) { \
    struct button_rule rule = {
        .mask = mask,
        .kill_multiple = kill_multiple,
        .press_event kj
}

bool buttons_init() {
    // vector XX
    int i = -1;
    rules_music[++i] = new_rule(
        (N_B | N_LEFT), false, do_b_left    // seek left
    );
    rules_music[++i] = new_rule(
        (N_B | N_RIGHT), false, do_b_right  // seek right
    );
    rules_music[++i] = new_rule(
        (N_B | N_UP), true, do_b_up         // playlist up
    );
    rules_music[++i] = new_rule(
        (N_B | N_DOWN), true, do_b_down     // playlist down
    );
    rules_music[++i] = new_rule(
        (      N_LEFT), true, do_left       // prev song
    );
    rules_music[++i] = new_rule(
        (      N_RIGHT), true, do_right     // next song
    );
    rules_music[++i] = new_rule(
        (      N_UP), true, do_up           // vol up
    );
    rules_music[++i] = new_rule(
        (      N_DOWN), true, do_down       // vol down
    );
    rules_music[++i] = new_rule(
        (      N_A), true, do_a             // random
    );

    i = -1;
    rules_general[++i] = new_rule(
        (      N_START), false, do_general_start // so we can hold down start for power off
    );

    return true;
}


