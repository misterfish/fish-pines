#include <stdbool.h>

bool mode_general();
bool mode_music();
bool mode_set_music();
bool mode_set_general();
bool mode_toggle();

static bool update_mode_led();
