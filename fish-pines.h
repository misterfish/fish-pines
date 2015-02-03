static unsigned int read_buttons_testing();

static void init_state();

static bool do_button(bool on, bool *state_ptr, bool kill_multiple, bool fn_on(), bool fn_off(), char *button_string) 
;

static void cleanup();

static int make_canonical(unsigned int read);

static char *debug_read_init();
static void debug_read(unsigned int read_canonical, char *ret);
//static char *debug_read(unsigned int read_canonical);

static bool do_read(unsigned int cur_read);
static bool process_read(unsigned int read, char *button_print);
static int get_max_button_print_size();

static bool get_kill_multiple(unsigned int read);

void main_set_mode(int mode);
