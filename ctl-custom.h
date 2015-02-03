#include <stdbool.h>

bool ctl_custom_init();

bool ctl_custom_b_left();
bool ctl_custom_b_right();
bool ctl_custom_b_up();
bool ctl_custom_b_down();
bool ctl_custom_left();
bool ctl_custom_right();
bool ctl_custom_up();
bool ctl_custom_down();
bool ctl_custom_a();
bool ctl_custom_select();
bool ctl_custom_start();

void ctl_custom_start_released();

static bool shell_go(char *cmd);
static bool shell_cmd_with_cb(char *cmd, void*(cb)());
static bool shell_cmd(char *cmd);
