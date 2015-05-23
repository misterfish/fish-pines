#define F_PIN_FORCE     0x01

bool gpio_init(bool verbose);
bool gpio_cleanup();

bool gpio_pin_read(int pin_gpio, int *state);
bool gpio_pin_on(int pin_gpio);
bool gpio_pin_off(int pin_gpio);
bool gpio_pin_on_f(int pin_gpio, int flags);
bool gpio_pin_off_f(int pin_gpio, int flags);

/* Caller shouldn't free ary.
 */
void gpio_get_phys_pins(int **ary, int *num_used, int *num_total);

int gpio_pin_read_l(lua_State *L /*int pin_gpio, int *state*/);
int gpio_pin_on_l(lua_State *L /*int pin_gpio*/);
int gpio_pin_off_l(lua_State *L /*int pin_gpio*/);

