#ifndef __INCL_BUTTONS_H
# error ctl.h needs buttons.h
#endif

bool ctl_init(bool do_uinput);

bool ctl_do_up();
bool ctl_do_down();
bool ctl_do_left();
bool ctl_do_right();
bool ctl_do_center_x();
bool ctl_do_center_y();
bool ctl_do_select_up();
bool ctl_do_select_down();
bool ctl_do_start_up();
bool ctl_do_start_down();
bool ctl_do_a_up();
bool ctl_do_a_down();
bool ctl_do_b_up();
bool ctl_do_b_down();

static bool update_mode_led();
static void set_mode(int mode);
