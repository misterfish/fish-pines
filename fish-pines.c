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

#ifdef DEBUG
    char *debug_read_s;
#endif
} g;

static bool _break = false;

void sighandler_term() {
    info("Ctl c");
    _break = true;
}

int main() {
    fish_utils_init();

    f_autoflush();

    if (! init_globals()) 
        ierr("Can't init globals.");

    init_state();

    if (! buttons_init()) 
        ierr("Couldn't init buttons");

    if (! vol_init()) 
        ierr("Couldn't init vol");

#ifndef NO_NES
    info("setting up wiringPi");

    if (! nes_init_wiring()) 
        ierr("Couldn't init wiring.");

    info("setting up nes");

    int joystick = nes_setup();
    if (joystick == -1) 
        ierr("Couldn't init joystick");
#else
    info("setting terminal raw");

    /* Not interbyte timeout, because 'nothing' should also be a valid
     * response.
     */
    if (!f_terminal_raw_input(F_UTIL_TERMINAL_MODE_READ_WITH_TIMEOUT, 0, POLL_TENTHS_OF_A_SECOND)) 
        err("Couldn't set terminal raw.");
#endif

    flua_config_set_verbose(true);

    if (! init_lua()) 
        err("Can't init lua.");

    info("setting up ctl + mpd");
    if (! f_mpd_init()) 
        err("Couldn't init mpd.");

    int first = 1;
    unsigned int cur_read;

    int i_mpd_update = 0;

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
        bool do_mpd_update = false;

        if (++i_mpd_update % MPD_UPDATE == 0) {
            i_mpd_update = 0;
            do_mpd_update = true;
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

        /* Check for mpd events.
         */
        if (do_mpd_update) 
            f_mpd_update();
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

    struct button_rule *rule = buttons_get_rule(read);

    bool kill_multiple = BUTTONS_KILL_MULTIPLE_DEFAULT;
    bool (*press_cb)() = NULL;
    if (rule) {
        kill_multiple = rule->kill_multiple;
        press_cb = rule->press_event;
    }

    if ((prev_read == read) && kill_multiple) {
#ifdef DEBUG
        info("Ignoring read %d (kill multiple is true)", read);
#endif
        return true;
    }

    if (button_print) 
        printf("[ %s ] ", button_print);

    /* Do the press and release events for each individual button. 
     * Then do the event matching the combination.
     */
    bool ok = true;
    for (int i = 0; i < 8; i++) {
        int button_flag = BUTTONS(i); // wiringPi system

        bool this_ok;
        /* Button down (press or hold).
         */
        if (read & button_flag) {
            // XX
        }

        /* Button not down.
         */
        else {
            /* Released.
             */
            if (prev_read & button_flag) {
                /*
                this_ok = (*event_release)();
                if (!this_ok) 
                    ok = false;
                    */
            }

            /* Was up and stayed up, do nothing.
             */
            else {
            }
        }
    }

    if (press_cb) {
        if (! (*press_cb)())
            pieprf;
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

    /*
    lua_pushstring(L, "creak");
    lua_pushcfunction(L, (lua_CFunction) creak);
    lua_rawset(L, -3);
    */

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

    lua_rawset(L, -3);  // buttons

    // capi.mpd = {
    lua_pushstring(L, "mpd");
    lua_newtable(L);

    lua_pushstring(L, "config_func");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_configl);
    lua_rawset(L, -3);  

    lua_rawset(L, -3);  // } capi.mpd

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


