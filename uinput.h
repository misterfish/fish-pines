bool uinput_init();

bool uinput_btn_a();
bool uinput_btn_b_up();
bool uinput_btn_b_down();
bool uinput_btn_start();
bool uinput_btn_select();
bool uinput_down();
bool uinput_up();
bool uinput_right();
bool uinput_left();
bool uinput_center_x();
bool uinput_center_y();

static void _delay();
static bool uinput_inject_key(int code, int val);
static bool uinput_inject_dir(int axis, int val);
