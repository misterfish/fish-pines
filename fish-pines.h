#ifdef NO_NES
static unsigned int read_buttons_testing();
static int make_canonical(unsigned int read);
static char *debug_read_init();
static void debug_read(unsigned int read_canonical, char *ret);
#endif

static void cleanup();

static bool do_read(unsigned int cur_read);
static bool process_read(unsigned int read, char *button_print);
static int get_max_button_print_size();

void main_set_mode(int mode);
