#define _GNU_SOURCE

#define VERBOSE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

/* rand
 */
#include <stdlib.h>

#include <signal.h>

#include <lua.h>
#include <lauxlib.h> // luaL_ functions.
#include <lualib.h> // luaL_ functions.
#include <luaconf.h> // luai_writestringerror

#include <fish-util.h>
#include <fish-utils.h>

#include "const.h"
#include "global.h"
#include "buttons.h"
#include "mpd.h"
#include "mode.h"
#include "vol.h"
#include "flua_config.h"
//#include "mode.h"
#include "util.h"

#ifndef NO_NES
# include "nes.h"
#endif

#include "fish-pines.h"

/* inlined in const.h */
extern short BUTTONS(short);

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

static bool init_globals();
static void init_state();
static bool init_lua();
static int get_max_button_print_size();
#ifdef DEBUG
static int make_canonical(unsigned int read);
static char *debug_read_init();
static void debug_read(unsigned int read_canonical, char *ret);
#endif

static struct {
    /* Structs have named fields 'a', 'start', 'down', etc.
     */
    struct button_name_s button_names;

    /* Std order.
     */
    char **button_name_iter[8];

    char *button_print;

    /* Elements are struct main_loop_event_t *.
     */
    vec *loop_events;
    int num_loop_events;

#ifdef DEBUG
    char *debug_read_s;
#endif
} g;

struct main_loop_event_t {
    char *desc;
    int gong; // how many ticks per gong
    int count; // how many ticks we're at
    bool (*cb)(void *data); // the callback
};

static bool _break = false;

void sighandler_term() {
    info("Ctl c");
    _break = true;
}

bool ping(void *data) {
    ++data;
    info("ping.");
    return true;
}

bool ping_fail(void *data) {
    ++data;
    info("bad ping.");
    return false;
}

/* desc is not dup'ed.
 */
void main_register_loop_event(char *desc, int count, bool (*cb)(void *data)) {
    struct main_loop_event_t *ev = f_mallocv(*ev);
    memset(ev, '\0', sizeof *ev);
    ev->desc = desc;
    ev->gong = count;
    ev->count = 0;
    ev->cb = cb;

    vec_add(g.loop_events, ev);
    g.num_loop_events++;
}

int main() {
    fish_utils_init();

    f_autoflush();

    if (! init_globals()) 
        ierr("Can't init globals.");

    init_state();

    /* For testing.
    main_register_loop_event("ping", 50, ping);
    main_register_loop_event("ping", 150, ping_fail);
    */

#ifndef NO_NES
    if (! nes_init_config())
        ierr("Couldn't init nes config");
#endif
    if (! f_mpd_init_config())
        ierr("Couldn't init mpd config");

    if (! mode_init_config())
        ierr("Couldn't init mode config");

    flua_config_set_verbose(true);

    /* Before init_lua.
     */
    if (! buttons_init()) 
        ierr("Couldn't init buttons");

    if (! init_lua()) 
        err("Can't init lua.");

#ifndef NO_NES
    info("Setting up wiringPi+nes.");

    int joystick = nes_init();
    if (joystick == -1)
        ierr("Couldn't init wiringPi/nes.");
#else
    info("setting terminal raw");

    /* Not interbyte timeout, because 'nothing' should also be a valid
     * response.
     */
    if (!f_terminal_raw_input(F_UTIL_TERMINAL_MODE_READ_WITH_TIMEOUT, 0, POLL_TENTHS_OF_A_SECOND)) 
        err("Couldn't set terminal raw.");
#endif

    if (! mode_init()) 
        ierr("Couldn't init mode");

    if (! vol_init()) 
        ierr("Couldn't init vol");

    info("setting up ctl + mpd");
    if (! f_mpd_init()) 
        err("Couldn't init mpd.");

    int first = 1;
    unsigned int cur_read = 0;

#ifdef DEBUG
    g.debug_read_s = debug_read_init();
#endif

    g.button_print = f_malloc(get_max_button_print_size() * sizeof(char));

    f_sig(SIGTERM, sighandler_term);
    f_sig(SIGINT, sighandler_term);

    /* Main loop.
     */
    while (1) {
        if (_break) 
            break; // ctl c

        for (int i = 0; i < g.num_loop_events; i++) {
            struct main_loop_event_t *ev = (struct main_loop_event_t *) vec_get(g.loop_events, i);
            if (++ev->count == ev->gong) {
                bool ok = (*ev->cb)(NULL);
                if (!ok) {
                    _();
                    BR(ev->desc);
                    warn("Error on loop event %s", _s);
                }
                ev->count = 0;
            }
        }

#ifdef NO_NES
        /* timeout happens in readkey
         */
        cur_read = read_buttons_testing();
#else
        /* sleep here
         */
        first = first ? 0 : 0 * usleep (1000 * POLL_MS);
        cur_read = nes_read(joystick);
#endif

        /* Do it also if cur_read is 0.
         */
        do_read(cur_read);
    }

    cleanup();
    exit(0);
}

#ifdef NO_NES
static unsigned int read_buttons_testing() {
    ssize_t s;
    char *buf = calloc(2, sizeof(char));
    int fd = 0; // stdin
    s = read(fd, buf, 1);

    int ret;
    if (s == -1) {
        ret = 0;
    }
    else {
        ret = 
            *buf == 'a' ? N_A :
            *buf == 'b' ? N_B :
            *buf == 'h' ? N_LEFT :
            *buf == 'l' ? N_RIGHT :
            *buf == 'j' ? N_DOWN :
            *buf == 'k' ? N_UP :
            *buf == 'H' ? N_B | N_LEFT :
            *buf == 'L' ? N_B | N_RIGHT :
            *buf == 'J' ? N_B | N_DOWN :
            *buf == 'K' ? N_B | N_UP :
            *buf == ',' ? N_SELECT :
            *buf == '.' ? N_START :
            0;
    }
    free(buf);
    return ret;
}
#endif

static bool do_read(unsigned int cur_read) {
#ifdef DEBUG
    if (cur_read) {
        /* our bit order
         */
        unsigned int cur_read_canonical = make_canonical(cur_read);
        debug_read(cur_read_canonical, g.debug_read_s);
        info(g.debug_read_s);
    }
#endif

    char **button_name_ptr;

    char *button_print = g.button_print;

    bool first = true;
    bool found_one = false;
    *button_print = '\0';
    for (int i = 0; i < 8; i++) {
        button_name_ptr = g.button_name_iter[i];
        bool on = cur_read & BUTTONS(i);
        if (on) {
            if (first) 
                first = false;
            else 
                strcat(button_print, " + ");
            found_one = true;
            strcat(button_print, *button_name_ptr); // -O- only 8
        }
    }

    if (!process_read(cur_read, 
        found_one ? button_print : NULL )) 
        pieprf;

    return true;
}

static bool process_read(unsigned int read, char *button_print) {
    static unsigned int prev_read = -1;

int mode = 0; // XX

    struct button_rule_t *rule_press = NULL;
    if (read)
        rule_press = buttons_get_rule_press(mode, read);
    struct button_rule_t *rule_release = NULL;

    bool kill_multiple = BUTTONS_KILL_MULTIPLE_DEFAULT;
    bool has_event_press = false, has_event_release = false;
    if (rule_press) {
        kill_multiple = rule_press->kill_multiple;
        has_event_press = rule_press->has_handler;
    }

    if ((prev_read == read) && kill_multiple) {
#ifdef DEBUG
        info("Ignoring read %d (kill multiple is true)", read);
#endif
        return true;
    }

    if (button_print) 
        printf("[ %s ] ", button_print);

    /* Do the release events for each button. 
     * Then do the event matching the combination.
     *
     * Note that even a 3-button combination followed by releasing one of
     * the 3 buttons will generate a release event for that button.
     */
    bool ok = true;
    for (int i = 0; i < 8; i++) {
        int button_flag = BUTTONS(i); // wiringPi system

        /* Button down (press or hold).
         */
        if (read & button_flag) {
        }

        /* Button not down.
         */
        else {
            /* It was down before (i.e., now released).
             */
            if (prev_read & button_flag) {
                if ((rule_release = buttons_get_rule_release(mode, button_flag))) {
                    if (rule_release->has_handler) {
                        int reg_idx = rule_release->handler;
                        lua_rawgeti(global.L, LUA_REGISTRYINDEX, reg_idx);
                        if (lua_pcall(global.L, 0, 0, 0)) {
                            ok = false;
                            piepc; // XX
                        }
                    }
                }
            }

            /* Was up and stayed up, do nothing.
             */
            else {
            }
        }
    }

    if (has_event_press) {
        int reg_idx = rule_press->handler;
        lua_rawgeti(global.L, LUA_REGISTRYINDEX, reg_idx);
        /* Don't set ok to false (it's not bad enough to bubble up as a
         * failed process_read()).
         */
        int rc;
        if (rc = lua_pcall(global.L, 0, 0, 0)) {
            const char *err = luaL_checkstring(global.L, -1);
            check_lua_err(rc, "Lua error on press event: %s", err);
        }
    }
    prev_read = read;
    return ok;
}

static void cleanup() {
#ifdef NO_NES
    info("setting terminal normal.");
    if (!f_terminal_normal()) 
        piep;
#endif
    fish_util_cleanup();
    fish_utils_cleanup();

    if (!f_mpd_cleanup())
        piep;
    if (!buttons_cleanup()) 
        piep;

    char **button_name_ptr = (char**) (&g.button_names);
    for (int i = 0; i < 8; i++) {
        free(*button_name_ptr);
        button_name_ptr++;
    }

    free(g.button_print);

#ifdef DEBUG
    free(g.debug_read_s);
#endif
}

static int panic(lua_State *L) {
    // Taken from panic function in lua source.
    (void)L;  /* to avoid warnings */
    fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n", lua_tostring(L, -1));
    return 0;  /* return to Lua to abort */
}

static bool init_globals() {
    lua_State *L = luaL_newstate();
    if (!L) {
        warn("Can't init lua.");
        return false;
    }
    global.L = L;
    return true;
}

void creak() {
    fprintf(stderr, "CREAK!\n");
}

static bool init_lua() {
    lua_State *L = global.L;
    /* Was set in luaL but overwrite it. 
     * Returns old panic function.
     */
    lua_atpanic(L, panic);
    luaL_openlibs(L); // void

    lua_newtable(L); // capi

    // capi.buttons = {
    lua_pushstring(L, "buttons"); 
    lua_newtable(L); 

    //                  left = F_LEFT,
    lua_pushstring(L, "left");
    lua_pushnumber(L, F_LEFT);
    lua_rawset(L, -3);
    //                  ...
    lua_pushstring(L, "right");
    lua_pushnumber(L, F_RIGHT);
    lua_rawset(L, -3);
    lua_pushstring(L, "up");
    lua_pushnumber(L, F_UP);
    lua_rawset(L, -3);
    lua_pushstring(L, "down");
    lua_pushnumber(L, F_DOWN);
    lua_rawset(L, -3);
    lua_pushstring(L, "select");
    lua_pushnumber(L, F_SELECT);
    lua_rawset(L, -3);
    lua_pushstring(L, "start");
    lua_pushnumber(L, F_START);
    lua_rawset(L, -3);
    lua_pushstring(L, "b");
    lua_pushnumber(L, F_B);
    lua_rawset(L, -3);
    lua_pushstring(L, "a");
    lua_pushnumber(L, F_A);
    lua_rawset(L, -3);
    lua_pushstring(L, "add_rule");
    lua_pushcfunction(L, (lua_CFunction) buttons_add_rulel);
    lua_rawset(L, -3);

    lua_rawset(L, -3);  // buttons

    // capi.mpd = {
    lua_pushstring(L, "mpd");
    lua_newtable(L);

    lua_pushstring(L, "config_func");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_configl);
    lua_rawset(L, -3);  

    lua_rawset(L, -3);
    // } 

    // capi.mode = {
    lua_pushstring(L, "mode");
    lua_newtable(L);

    lua_pushstring(L, "config_func");
    lua_pushcfunction(L, (lua_CFunction) mode_configl);
    lua_rawset(L, -3);  
    lua_pushstring(L, "next_mode");
    lua_pushcfunction(L, (lua_CFunction) mode_next_model);
    lua_rawset(L, -3);  
    lua_pushstring(L, "get_mode_name");
    lua_pushcfunction(L, (lua_CFunction) mode_get_mode_namel);
    lua_rawset(L, -3);  

    lua_rawset(L, -3);
    // } 

    // capi.nes = {
    lua_pushstring(L, "nes");
    lua_newtable(L);
    lua_pushstring(L, "config_func");
    lua_pushcfunction(L, (lua_CFunction) nes_configl);
    lua_rawset(L, -3);  
    lua_rawset(L, -3);
    // }

    lua_setglobal(L, "capi");


    if (luaL_loadfile(L, "init.lua")) {
        warn("Couldn't load lua init script: %s", lua_tostring(L, -1));
        return false;
    }

    if (lua_pcall(L, 0, LUA_MULTRET, 0)) {
        warn("Failed to run script: %s\n", lua_tostring(L, -1));
        return false;
    }

    return true;
}

static void init_state() {
    g.loop_events = vec_new();
    f_track_heap(g.loop_events);

    g.button_names.left = G_("left");
    g.button_names.right = G_("right");
    g.button_names.up = G_("up");
    g.button_names.down = G_("down");
    g.button_names.select = Y_("select");
    g.button_names.start = Y_("start");
    g.button_names.b = CY_("B");
    g.button_names.a = CY_("A");

    g.button_name_iter[0] = &g.button_names.left;
    g.button_name_iter[1] = &g.button_names.right;
    g.button_name_iter[2] = &g.button_names.up;
    g.button_name_iter[3] = &g.button_names.down;
    g.button_name_iter[4] = &g.button_names.select;
    g.button_name_iter[5] = &g.button_names.start;
    g.button_name_iter[6] = &g.button_names.b;
    g.button_name_iter[7] = &g.button_names.a;
}

#ifdef DEBUG
static int make_canonical(unsigned int read) {
    return 
        ((read & N_LEFT)    ? 1<<7 : 0) |
        ((read & N_RIGHT)   ? 1<<6 : 0) |
        ((read & N_UP)      ? 1<<5 : 0) |
        ((read & N_DOWN)    ? 1<<4 : 0) |
        ((read & N_SELECT)  ? 1<<3 : 0) |
        ((read & N_START)   ? 1<<2 : 0) |
        ((read & N_B)       ? 1<<1 : 0) |
        ((read & N_A)       ? 1<<0 : 0);
}

static char *debug_read_init() {
    char *ret = malloc(
        9
        * sizeof(char)
    );
    ret[8] = '\0';
    return ret;
}

static void debug_read(unsigned int read_canonical, char *ret) {
    char *ptr = ret;
    for (int shift = 7; shift >= 0; shift--) {
        *ptr++ = (read_canonical & (1<<shift)) ? '1' : '0';
    }
}
#endif

static int get_max_button_print_size() {
    int n=0;
    for (int i = 0; i < 8; i++) {
        n += strlen(*g.button_name_iter[i]) 
            + 3; // ' + '
    }
    return n+1;
}


