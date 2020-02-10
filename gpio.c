#include <lua.h>
#include <lauxlib.h>

#include <fish-util.h>

#include "gpio.h"

#define DIR_OUT 0x01
#define DIR_IN  0x02

#define DIR_OUT_STRING "out"
#define DIR_IN_STRING "in"
#define DIR_OUT_STRING_LENGTH 3
#define DIR_IN_STRING_LENGTH 2

#define MAX_GPIO 27 // maximum physical number (26) plus 1
#define PHYS_PINS_USED 17
#define PHYS_PINS_TOTAL 26
#define NUM_PINS 40

static char *DIR = "/sys/class/gpio";

static int PHYS_PINS[] = {
    3, 5, 7, 8, 
    10, 11, 12, 13, 15, 16, 
    18, 19, 21, 22, 23, 24, 
    26
};
static int phys2gpio(int pin) {
    if (pin == 3) return 2; // 0 or 2, we use 2
    if (pin == 5) return 3; // 1 or 3, we use 3
    if (pin == 7) return 4;
    if (pin == 8) return 14;
    if (pin == 10) return 15;
    if (pin == 11) return 17;
    if (pin == 12) return 18;
    if (pin == 13) return 27;
    if (pin == 15) return 22;
    if (pin == 16) return 23;
    if (pin == 18) return 24;
    if (pin == 19) return 10;
    if (pin == 21) return 9;
    if (pin == 22) return 25;
    if (pin == 23) return 11;
    if (pin == 24) return 8;
    if (pin == 26) return 7;
    else return -1;
}
/*
static int gpio2phys(int pin) {
    if (pin == 2) return 3;
    else if (pin == 3) return 5;
    else if (pin == 4) return 7;
    else if (pin == 14) return 8;
    else if (pin == 15) return 10;
    else if (pin == 17) return 11;
    else if (pin == 18) return 12;
    else if (pin == 27) return 13;
    else if (pin == 22) return 15;
    else if (pin == 23) return 16;
    else if (pin == 24) return 18;
    else if (pin == 10) return 19;
    else if (pin == 9) return 21;
    else if (pin == 25) return 22;
    else if (pin == 11) return 23;
    else if (pin == 8) return 24;
    else if (pin == 7) return 26;
    else return -1;
};
*/

static struct {
    bool verbose;
    char *fileexport;

    /* Cache the states. Assume no one outside of us is setting pins; if
     * not, use the FORCE flag when turning a pin on or off.
     */
    int state[NUM_PINS]; 
} g;

static int phys_to_gpio(int pin_phys);
static char *get_directory(int pin_gpio);
static char *get_state_file(int pin_gpio);
static bool isexported(int pin_gpio);
static bool export(int pin_gpio);
static bool get_dir(int pin_gpio, int *store);
//static bool is_dir_in(int pin_gpio);
static bool is_dir_out(int pin_gpio);
static bool set_dir(int pin_gpio, int dir);
static bool pin_read(int pin_gpio, int *state);
//static bool pin_write(int pin_phys, int pin_gpio, int state);
static bool pin_write(int pin_gpio, int state);

static int phys_to_gpio(int pin_phys) {
    if (pin_phys < 1 || pin_phys > MAX_GPIO - 1) {
        _();
        spr("%d", pin_phys);
        R(_s);
        warn("Physical pin not between 1 and %d (%s)", MAX_GPIO - 1, _t);
        return -1;
    }

    int pin_gpio = phys2gpio(pin_phys);
    if (pin_gpio == -1) {
        _();
        spr("%d", pin_phys);
        R(_s);
        warn("Physical pin %s is not a GPIO pin", _t);
        return -1;
    }

    return pin_gpio;
}
/*
static int gpio_to_phys(int pin_gpio) {
    if (pin_gpio < 1 || pin_gpio > MAX_GPIO) {
        _();
        spr("%d", pin_gpio);
        R(_s);
        warn("-p not between 1 and %d (%s)", MAX_GPIO, _t);
        return -1;
    }
    int pin_phys = gpio2phys(pin_gpio);
    if (pin_phys == -1) {
        _();
        spr("%d", pin_gpio);
        R(_s);
        warn("GPIO pin %s not recognised.", _t);
        return -1;
    }

    return pin_phys;
}
*/

/* Caller should free.
 */
static char *get_directory(int pin_gpio) {
    char *dir = f_malloc(strlen(DIR) + 1 + 5 + 2);
    sprintf(dir, "%s/gpio%d", DIR, pin_gpio);

    return dir;
}

/* Caller should free.
 */
static char *get_state_file(int pin_gpio) {
    char *dir = get_directory(pin_gpio);
    char *file = f_malloc(strlen(dir) + 1 + 6);
    sprintf(file, "%s/value", dir);
    free(dir);
    return file;
}

static bool isexported(int pin_gpio) {
#ifndef TESTING_OFF_PI
    char *dir = get_directory(pin_gpio);
    bool ret = f_test_d(dir);
    free(dir);
#else
    bool ret = true;
#endif
    return ret;
}

static bool export(int pin_gpio) {
    char *file = g.fileexport;
    FILE *f = safeopen_f(file, F_WRITE | F_NODIE);
    if (!f) 
        pieprf;
    fprintf(f, "%d", pin_gpio);
    if (fclose(f)) 
        pieprf;
    return true;
}

/* Caller should free.
 */
static char *get_file_direction(int pin_gpio) {
    char *dir = get_directory(pin_gpio);
    char *file = f_malloc(strlen(dir) + 1 + 10);
    sprintf(file, "%s/direction", dir);
    free(dir);
    return file;
}

static bool get_dir(int pin_gpio, int *store) {
    char *file = get_file_direction(pin_gpio);
    FILE *f = safeopen_f(file, F_READ | F_NODIE);
    if (!f) 
        pieprf;
    char *line = str(4);
    if (!fgets(line, 4, f))  // reads at most one less than 4, then adds \0
        pieprf;
    if (fclose(f))
        pieprf;

    bool ok = true;
    if (!strncmp(line, DIR_IN_STRING, DIR_IN_STRING_LENGTH))
        *store = DIR_IN;
    else if (!strncmp(line, DIR_OUT_STRING, DIR_OUT_STRING_LENGTH))
        *store = DIR_OUT;
    else {
        _();
        Y(line);
        warn("Unexpected: got %s", _s);
        ok = false;
    }
    free(file);
    free(line);
    return ok;
}

static bool is_dir_out(int pin_gpio) {
#ifndef TESTING_OFF_PI
    int dir;
    if (!get_dir(pin_gpio, &dir)) 
        pieprf;

    if (dir == DIR_OUT) {
        return true;
    }
    else if (dir == DIR_IN) {
        return false;
    }
#endif
    return true;
}

static bool set_dir(int pin_gpio, int dir) {
    char *file = get_file_direction(pin_gpio);

    FILE *f = safeopen_f(file, F_WRITE | F_NODIE);
    if (!f) 
        pieprf;
    char* s;
    if (dir == DIR_OUT) {
        s = DIR_OUT_STRING;
    }
    else if (dir == DIR_IN) {
        s = DIR_IN_STRING;
    }
    else {
        pieprf;
    }

    fprintf(f, "%s", s);
    if (fclose(f)) 
        pieprf;

    free(file);
    return true;
}

static bool pin_read(int pin_gpio, int *state) {
    if (! isexported(pin_gpio)) {
        if (! export(pin_gpio)) 
            pieprf;
    }

    if (! is_dir_out(pin_gpio)) {
        if (! set_dir(pin_gpio, DIR_OUT)) 
            pieprf;
    }

#ifndef TESTING_OFF_PI
    char *file = get_state_file(pin_gpio);
    FILE *f = safeopen_f(file, F_NODIE);

    if (!f) 
        pieprf;

    char c;
    int rc = fread(&c, sizeof(char), 1, f);
    if (rc != 1) 
        pieprf;
    if (c == '0') 
        *state = 0;
    else if (c == '1') 
        *state = 1;
    else {
        piep;
        *state = 0;
    }

#else
    int pin_phys = gpio_to_phys(pin_gpio);
    *state = g.state[pin_gpio];
#endif
    if (fclose(f)) 
        pieprf;
    free(file);
    return true;
}

static bool pin_write(int pin_gpio, int state) {
    if (state < 0 || state > 1)
        pieprf;

    if (! isexported(pin_gpio)) {
        if (! export(pin_gpio)) 
            pieprf;
    }

    if (! is_dir_out(pin_gpio)) {
        if (! set_dir(pin_gpio, DIR_OUT)) 
            piep;
    }

    if (g.verbose) {
        _();
        spr("%2d", pin_gpio);
        G(_s);

        if (state) G("on");
        else R("off");

#ifndef TESTING_OFF_PI
        char *left_quote = "";
        char *right_quote = "";
#else
        char *left_quote = "“";
        char *right_quote = "”";
#endif
        info("%sSetting%s gpio %s -> [%s]", left_quote, right_quote, _t, _u);
    }

#ifndef TESTING_OFF_PI
    char *file = get_state_file(pin_gpio);
    FILE *f = safeopen_f(file, F_WRITE | F_NODIE);
    if (!f)
        piep;

    fprintf(f, "%d", state);
    if (fclose(f)) 
        pieprf;
    free(file);
#endif

    // int pin_phys = gpio_to_phys(pin_gpio);
    g.state[pin_gpio] = state;

    return true;
}

/* Public.
 */

bool gpio_init(bool verbose) {
    g.verbose = verbose;

    char *fe = f_malloc(strlen(DIR) + 1 + 7);
    sprintf(fe, "%s/export", DIR);
    g.fileexport = fe;

    for (int i = 0; i < PHYS_PINS_USED; i++) {
        int state = 0;
        int pin_phys = PHYS_PINS[i];
        int pin_gpio = phys_to_gpio(pin_phys);
        if (pin_gpio == -1)
            pieprf;
        if (!gpio_pin_read(pin_gpio, &state)) {
            piepc;
        }
        g.state[pin_gpio] = state;
    }
    return true;
}

bool gpio_cleanup() {
    free(g.fileexport);
    return true;
}

//bool gpio_pin_read(int pin_phys, int *state) {
bool gpio_pin_read(int pin_gpio, int *state) {
    //int pin_gpio = phys_to_gpio(pin_phys);

    if (pin_gpio == -1) 
        pieprf;

    return pin_read(pin_gpio, state);
}

//bool gpio_pin_on_f(int pin_phys, int flags) {
bool gpio_pin_on_f(int pin_gpio, int flags) {
    //int pin_gpio = phys_to_gpio(pin_phys);

    // int pin_phys = gpio_to_phys(pin_gpio);
    if (pin_gpio == -1) 
        pieprf;

    if (flags & F_PIN_FORCE) {
        // Don't check state.
    }
    else if (g.state[pin_gpio])
        return true;

    return pin_write(pin_gpio, 1);
}

//bool gpio_pin_off_f(int pin_phys, int flags) {
bool gpio_pin_off_f(int pin_gpio, int flags) {
    //int pin_gpio = phys_to_gpio(pin_phys);

    if (pin_gpio == -1) 
        pieprf;

    // int pin_phys = gpio_to_phys(pin_gpio);

    if (flags & F_PIN_FORCE) {
        // Don't check state.
    }
    else if (!g.state[pin_gpio])
        return true;

    return pin_write(pin_gpio, 0);
}

//bool gpio_pin_off(int pin_phys) {
bool gpio_pin_off(int pin_gpio) {
    //return gpio_pin_off_f(pin_phys, 0);
    return gpio_pin_off_f(pin_gpio, 0);
}

//bool gpio_pin_on(int pin_phys) {
bool gpio_pin_on(int pin_gpio) {
    //return gpio_pin_on_f(pin_phys, 0);
    return gpio_pin_on_f(pin_gpio, 0);
}

void gpio_get_phys_pins(int **ary, int *num_used, int *num_total) {
    *ary = PHYS_PINS;
    *num_used = PHYS_PINS_USED;
    *num_total = PHYS_PINS_TOTAL;
}

/* Lua.
 */

int gpio_pin_read_l(lua_State *L /*int pin_gpio*/) {
    int pin_gpio = (int) luaL_checknumber(L, -1);
    lua_pop(L, 1);
    int state;
    //if (! gpio_pin_read(pin_phys, &state)) {
    if (! gpio_pin_read(pin_gpio, &state)) {
        piep;
        return 0;
    }
    lua_pushnumber(L, state);
    return 1;
}

int gpio_pin_on_l(lua_State *L) {
    int pin_gpio, flags = 0;
    int numargs = lua_gettop(L);
    if (numargs == 2) {
        const char *flag = luaL_checkstring(L, -1);
        lua_pop(L, 1);
        if (! strcmp(flag, "force"))
            flags |= F_PIN_FORCE;
        else
            piepr0;
        lua_pop(L, 1);
    }

    pin_gpio = (int) luaL_checknumber(L, -1);
    lua_pop(L, 1);
    if (! gpio_pin_on_f(pin_gpio, flags)) 
        piep;
    return 0;
}

int gpio_pin_off_l(lua_State *L) {
    int pin_gpio, flags = 0;
    int numargs = lua_gettop(L);
    if (numargs == 2) {
        const char *flag = luaL_checkstring(L, -1);
        lua_pop(L, 1);
        if (! strcmp(flag, "force"))
            flags |= F_PIN_FORCE;
        else
            piepr0;
        lua_pop(L, 1);
    }
    pin_gpio = (int) luaL_checknumber(L, -1);
    lua_pop(L, 1);
    if (! gpio_pin_off_f(pin_gpio, flags)) 
        piep;
    return 0;
}


