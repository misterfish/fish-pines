#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#include <glib.h>
#include <lua.h>
#include <lauxlib.h> // luaL_ functions.
#include <lualib.h> // luaL_ functions.
#include <luaconf.h> // luai_writestringerror

#include <fish-util.h>
#include <fish-utils.h>

#include "const.h"
#include "global.h"
#include "arg.h"
#include "buttons.h"
#include "gpio.h"
#include "mpd.h"
#include "mode.h"
#include "vol.h"
#include "flua_config.h"
#include "util.h"

#ifndef NO_NES
# include "nes.h"
#endif

#include "fish-pines.h"

#define CONF_NAMESPACE "main"
#define CONF_DEFAULT_POLL_MS 40

#define MAIN_EVENT_RANDOM   0x01

struct timeout_lua_t {
    lua_State *L;
    int reg_index;
    bool repeat;
};

const char *EVENTS[] = {
    "random",
    "playlists-changed",
    "update-started-or-finished",
    "database-updated",
    "player-state-changed",
    "volume-altered",
    "device-state-changed",
    "sticker-modified",
    "client-channel-subscription-altered",
    "subscribed-channel-message-received"
};

/* inlined in const.h */
extern short BUTTONS(short);

#ifdef NO_NES
static unsigned int read_buttons_testing();
static int make_canonical(unsigned int read);
static char *debug_read_init();
static void debug_read(unsigned int read_canonical, char *ret);
#endif

static void print_hold_indicator(short s);
static void cleanup();

static bool do_read(short cur_read);
static bool process_read(short read, char *button_print);
static int get_max_button_print_size();

static int add_listener_l(lua_State *L);
static int add_timeout_l(lua_State *L);
static int remove_timeout_l(lua_State *L);

static bool init_globals();
static bool init_main(int argc, char **argv, int *way_out);

static bool lua_init();
static bool lua_start();

static int get_max_button_print_size();
#ifdef DEBUG
static int make_canonical(unsigned int read);
static char *debug_read_init();
static void debug_read(unsigned int read_canonical, char *ret);
#endif

static int config_l(lua_State *L);

//static union arg_union_t args[ARG_last];

static struct {
    struct flua_config_conf_t *conf;
    bool lua_initted;

    /* Note, can't use it until after lua_init */
    bool verbose;

    int joystick;

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

    GHashTable *events;

#ifdef DEBUG
    char *debug_read_s;
#endif
} g;

static struct flua_config_conf_item_t CONF[] = {
    flua_conf_default(poll_ms, integer, CONF_DEFAULT_POLL_MS)
    flua_conf_default(verbose, boolean, false)

    flua_conf_last
};

static bool main_init_config() {
    g.conf = flua_config_new(global.L);
    if (!g.conf)
        pieprf;
    flua_config_set_namespace(g.conf, CONF_NAMESPACE);
    return true;
}

static bool timeout_lua_func(gpointer ptr) {
    struct timeout_lua_t *data = (struct timeout_lua_t *) ptr;
    int reg_index = data->reg_index;
    lua_State *L = data->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, reg_index);

    int rc;
    if ((rc = lua_pcall(L, 0, 1, 0))) {
        const char *err = luaL_checkstring(global.L, -1);
        check_lua_err(rc, "Lua error on user-defined timeout: %s", err);
    }
    bool l_return;
    if (! strcmp(lua_typename(L, lua_type(L, -1)), "nil")) 
        l_return = false;
    else 
        l_return = lua_toboolean(L, -1);

    return l_return && data->repeat;
}

static bool add_timeout_lua_func(int ms, lua_State *L, int reg_index, bool repeat, guint *id) {
    struct timeout_lua_t *data = (struct timeout_lua_t *) f_mallocv(*data);
    memset(data, 0, sizeof(*data));
    data->reg_index = reg_index;
    data->L = L;
    data->repeat = repeat;

    // can't fail
    guint tid = main_add_timeout(ms, timeout_lua_func, data);
    if (id) 
        *id = tid;
    return true;
}

static void sighandler_term() {
    say("");
    info("Ctl c");
}

static void poll_nes(gpointer data) {
    (void) data;
    unsigned int cur_read = 0;

#ifdef NO_NES
    /* readkey sleeps (check) XX */
    cur_read = read_buttons_testing();
#else
    cur_read = nes_read(g.joystick);
#endif

    /* Do it also if cur_read is 0 (so we can get release events). */
    do_read(cur_read);
}

/* For testing timeouts.
 */
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

void main_register_loop_event(char *desc, int ms, bool (*cb)(void *data)) {
    /* Not doing anything with the desc actually. */
    /* Or with the timeout id. */
    guint id = g_timeout_add(ms, (GSourceFunc) cb, NULL);
    (void) desc;
    (void) id;
}

/* Lua functions have a _l suffix throughout the app.
 * They can 'throw' with lua_error, which is safe -- it will cancel the lua
 * call and return control to the main loop.
 *
 * Conversely, C can call lua functions by putting them on the lua stack and
 * using pcall. Then we have a macro which handles the error, dying if it's
 * an OOM error, or warning about it. Then we return false to the calling C
 * function.
 */

/* Try not to be verbose until after lua_int */

int main(int argc, char **argv) {
    (void) argc;

    fish_utils_init();

    f_autoflush();

    if (! init_globals()) 
        ierr("Can't init globals.");

    /* For testing.
    main_register_loop_event("ping", 50, ping);
    main_register_loop_event("ping", 150, ping_fail);
    */

    if (! main_init_config())
        ierr("Couldn't init main config");

    int way_out;
    if (! init_main(argc, argv, &way_out)) {
        if (way_out == 0)
            exit(0);
        if (way_out == 1)
            exit(1);
        else 
            ierr("Couldn't init main.");
    }

#ifndef NO_NES
    if (! nes_init_config())
        ierr("Couldn't init nes config");
#endif
    if (! f_mpd_init_config())
        ierr("Couldn't init mpd config");

    if (! mode_init_config())
        ierr("Couldn't init mode config");

    if (! vol_init_config())
        ierr("Couldn't init mode config");

    flua_config_set_verbose(true);

    if (! lua_init())
        err("Can't init lua.");

    if (! g.lua_initted) 
        err("%s: forgot lua init?", CONF_NAMESPACE);

    g.verbose = conf_b(verbose);

    if (! gpio_init(g.verbose)) 
        ierr("Couldn't init gpio");

    if (g.verbose) 
        say("");
#ifndef NO_NES
    if (g.verbose)
        info("Setting up wiringPi+nes.");

    g.joystick = nes_init();
    if (g.joystick == -1)
        ierr("Couldn't init wiringPi/nes.");
#else
    if (g.verbose)
        info("setting terminal raw");

    /* Not interbyte timeout, because 'nothing' should also be a valid
     * response.
     */
    if (!f_terminal_raw_input(F_UTIL_TERMINAL_MODE_READ_WITH_TIMEOUT, 0, POLL_TENTHS_OF_A_SECOND)) 
        err("Couldn't set terminal raw.");
#endif

    /* Before buttons_init, which needs to know about the modes. */
    if (! mode_init()) 
        ierr("Couldn't init mode");

    if (! buttons_init()) 
        ierr("Couldn't init buttons");

    if (g.verbose){
        info("Setting up vol (fasound)");
        say("");
    }

    if (! vol_init()) 
        ierr("Couldn't init vol");

    if (g.verbose) {
        say("");
        info("Setting up mpd.");
    }

    if (! f_mpd_init()) 
        err("Couldn't init mpd.");

    if (! lua_start()) 
        err("Can't call lua start().");

#ifdef DEBUG
    g.debug_read_s = debug_read_init();
#endif

    g.button_print = f_malloc(get_max_button_print_size() * sizeof(char));

    (void) sighandler_term;
    //f_sig(SIGTERM, sighandler_term);
    //f_sig(SIGINT, sighandler_term);

    /* Using glib as the main loop doesn't add anything useful in terms of
     * I/O -- we still poll the pins using wiringPi each time around.
     * But it does help in easily adding timeout functions.
     */
  
    GMainLoop *loop;
    {
        GMainContext *ctxt = NULL;
        gboolean is_running = false;
        loop = g_main_loop_new(ctxt, is_running);
    }

    guint poll_timeout = g_timeout_add(conf_i(poll_ms), (GSourceFunc) poll_nes, NULL);
    (void) poll_timeout;

    g_main_loop_run( loop );

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

static bool do_read(short cur_read) {
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

static bool process_read(short read, char *button_print) {
    static short prev_read = 0;

    static vec *rules_press = NULL;
    static vec *rules_release = NULL;
    if (!rules_press) 
        rules_press = vec_new();
    else if (! vec_clear(rules_press))
        pieprf;

    if (!rules_release) 
        rules_release = vec_new();
    else if (! vec_clear(rules_release))
        pieprf;

    short mode = mode_get_mode();

    if (read) {
        /* This is where ->exact gets applied. */
        if (! buttons_get_rules_press(mode, read, rules_press))
            pieprf;
    }

    if ((prev_read != read) && button_print) {
        /* reset it */
        print_hold_indicator(0); 
        printf("[ %s ] ", button_print);
    }

    /* Cycle through individual buttons, triggering their release events if
     * they have them. 
     *
     * Note that an n-button combination followed by releasing one of
     * the buttons will generate a release event for that button (followed
     * by a press event for the (n-1) combination.
     *
     * Also, 'once', 'chain' and 'exact' only apply to press events.
     */

    bool ok = true;
    for (int i = 0; i < 8; i++) {
        int button_flag = BUTTONS(i); // wiringPi system

        /* Button down (press or hold) -- do nothing */
        if (read & button_flag) {
        }

        /* Button not down. */
        else {
            /* It was down before (i.e., now released). */
            if (prev_read & button_flag) {
                if (! buttons_get_rules_release(mode, button_flag, rules_release)) {
                    piepc;
                }

                int j, l;
                for (j = 0, l = vec_size(rules_release); j < l; j++) {
                    struct button_rule_t *rule_release = (struct button_rule_t *) vec_get(rules_release, j);
                    if (! rule_release) {
                        piepc;
                    }

                    if (rule_release->has_handler) {
                        int reg_idx = rule_release->handler;
                        lua_rawgeti(global.L, LUA_REGISTRYINDEX, reg_idx);

                        /* Don't set ok to false -- not serious enough. */
                        int rc;
                        if ((rc = lua_pcall(global.L, 0, 0, 0))) {
                            const char *err = luaL_checkstring(global.L, -1);
                            check_lua_err(rc, "Lua error on press event: %s", err);
                            continue;
                        }
                    }
                }
            }

            /* Was up and stayed up -- do nothing. */
            else {
            }
        }
    }

    if (! read) 
        goto END;

    /* Then do the event matching the combination. */
    int j = 0, l = vec_size(rules_press);

    if (l == 0) 
        print_hold_indicator(1);
    // 'else'

    for (; j < l; j++) {
        struct button_rule_t *rule_press = (struct button_rule_t *) vec_get(rules_press, j);
        if (! rule_press) {
            piepc;
        }
        if (rule_press->once && prev_read == read) {
#ifdef DEBUG
            info("Ignoring read %d (->once is true)", read);
#endif
            if (! rule_press->chain) 
                break;
            else
                continue;
        }

        if (prev_read == read && rule_press->hold_indicator)
            print_hold_indicator(1);

        if (rule_press->has_handler) {
            int reg_idx = rule_press->handler;
            lua_rawgeti(global.L, LUA_REGISTRYINDEX, reg_idx);

            /* Don't set ok to false -- not serious enough. */
            int rc;
            if ((rc = lua_pcall(global.L, 0, 0, 0))) {
                const char *err = luaL_checkstring(global.L, -1);
                check_lua_err(rc, "Lua error on press event: %s", err);
            }
        }
        if (! rule_press->chain) 
            break;
    }
END:
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

    if (! f_mpd_cleanup())
        piep;
    if (! buttons_cleanup()) 
        piep;

    if (! vec_destroy(g.loop_events))
        piep;
            
    g_hash_table_destroy(g.events);

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

static bool lua_init() {
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
    lua_pushcfunction(L, (lua_CFunction) buttons_add_rule_l);
    lua_rawset(L, -3);

    lua_rawset(L, -3);  // buttons

    // capi.main = {
    lua_pushstring(L, "main");
    lua_newtable(L);

    lua_pushstring(L, "config");
    lua_pushcfunction(L, (lua_CFunction) config_l);
    lua_rawset(L, -3);  

    lua_pushstring(L, "add_listener");
    lua_pushcfunction(L, (lua_CFunction) add_listener_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "add_timeout");
    lua_pushcfunction(L, (lua_CFunction) add_timeout_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "remove_timeout");
    lua_pushcfunction(L, (lua_CFunction) remove_timeout_l);
    lua_rawset(L, -3);

    lua_rawset(L, -3);
    // } 

    // capi.mpd = {
    lua_pushstring(L, "mpd");
    lua_newtable(L);

    lua_pushstring(L, "config");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_config_l);
    lua_rawset(L, -3);  

    lua_pushstring(L, "toggle_play");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_toggle_play_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "prev_song");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_prev_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "next_song");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_next_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "toggle_random");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_toggle_random_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "get_random");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_get_random_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "random_off");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_random_off_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "random_on");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_random_on_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "database_update");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_database_update_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "is_updating");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_is_updating_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "next_playlist");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_next_playlist_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "prev_playlist");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_prev_playlist_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "load_playlist_by_name");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_load_playlist_by_name_l);
    lua_rawset(L, -3);

    lua_pushstring(L, "seek");
    lua_pushcfunction(L, (lua_CFunction) f_mpd_seek_l);
    lua_rawset(L, -3);

    lua_rawset(L, -3);
    // } 

    // capi.vol = {
    lua_pushstring(L, "vol");
    lua_newtable(L);

    lua_pushstring(L, "config");
    lua_pushcfunction(L, (lua_CFunction) vol_config_l);
    lua_rawset(L, -3);  

    lua_pushstring(L, "set_rel");
    lua_pushcfunction(L, (lua_CFunction) vol_rel_l);
    lua_rawset(L, -3);  

    lua_rawset(L, -3);
    // } 

    // capi.gpio = {
    lua_pushstring(L, "gpio");
    lua_newtable(L);

    lua_pushstring(L, "read");
    lua_pushcfunction(L, (lua_CFunction) gpio_pin_read_l);
    lua_rawset(L, -3);  

    lua_pushstring(L, "on");
    lua_pushcfunction(L, (lua_CFunction) gpio_pin_on_l);
    lua_rawset(L, -3);  

    lua_pushstring(L, "off");
    lua_pushcfunction(L, (lua_CFunction) gpio_pin_off_l);
    lua_rawset(L, -3);  

    lua_rawset(L, -3);
    // } 

    // capi.mode = {
    lua_pushstring(L, "mode");
    lua_newtable(L);

    lua_pushstring(L, "config");
    lua_pushcfunction(L, (lua_CFunction) mode_config_l);
    lua_rawset(L, -3);  
    lua_pushstring(L, "next_mode");
    lua_pushcfunction(L, (lua_CFunction) mode_next_mode_l);
    lua_rawset(L, -3);  
    lua_pushstring(L, "get_mode_name");
    lua_pushcfunction(L, (lua_CFunction) mode_get_mode_name_l);
    lua_rawset(L, -3);  

    lua_rawset(L, -3);
    // } 

    // capi.util = {
    lua_pushstring(L, "util");
    lua_newtable(L);

    lua_pushstring(L, "get_clock");
    lua_pushcfunction(L, (lua_CFunction) util_get_clock_l);
    lua_rawset(L, -3);  

    lua_pushstring(L, "close_fd");
    lua_pushcfunction(L, (lua_CFunction) util_close_fd_l);
    lua_rawset(L, -3);  

    lua_pushstring(L, "redirect_write_to_dev_null");
    lua_pushcfunction(L, (lua_CFunction) util_write_fd_to_dev_null_l);
    lua_rawset(L, -3);  

    lua_pushstring(L, "socket_unix_message");
    lua_pushcfunction(L, (lua_CFunction) util_socket_unix_message_l);
    lua_rawset(L, -3);  

    lua_rawset(L, -3);
    // } 



#ifndef NO_NES
    // capi.nes = {
    lua_pushstring(L, "nes");
    lua_newtable(L);
    lua_pushstring(L, "config");
    lua_pushcfunction(L, (lua_CFunction) nes_config_l);
    lua_rawset(L, -3);  
    lua_rawset(L, -3);
    // }
#endif

    lua_setglobal(L, "capi");

    char *lua_dir = opts[ARG_lua_dir].string;
    if (chdir(lua_dir)) {
        _();
        BR(lua_dir);
        warn("Couldn't cd to %s", _s);
        return false;
    }

    char *buf = f_malloc(sizeof(char) * strlen(LUA_RC) + 2 + 1);
    sprintf(buf, "./%s", LUA_RC);

    if (luaL_loadfile(L, buf)) {
        warn("Couldn't load lua init script: %s", lua_tostring(L, -1));
        return false;
    }
    free(buf);

    int rc;
    if ((rc = lua_pcall(L, 0, LUA_MULTRET, 0))) {
        const char *err = luaL_checkstring(L, -1);
        check_lua_err(rc, "Failed to run '%s'", lua_dir, err);
        return false;
    }

    return true;
}

static bool lua_start() {
    lua_State *L = global.L;
    lua_getglobal(L, LUA_RC_BOOTSTRAP);
    int rc;
    if ((rc = lua_pcall(L, 0, LUA_MULTRET, 0))) {
        const char *err = luaL_checkstring(L, -1);
        check_lua_err(rc, "Failed to run lua function '%s': %s", LUA_RC_BOOTSTRAP, err);
        return false;
    }
    return true;
}

static void events_destroy_val(gpointer ptr) {
    if (! vec_destroy((vec *) ptr))
        piep;
}

static bool init_main(int argc, char **argv, int *way_out) {
    //if (! arg_args(argc, argv, args)) {
    if (! arg_args(argc, argv)) {
        int status = arg_status();
        if (status == ARG_STATUS_HELP) 
            *way_out = 0;
        else if (status == ARG_STATUS_INVALID_USAGE) 
            *way_out = 1;
        else {
            iwarn("Couldn't init args.");
            *way_out = 1;
        }
        return false;
    }

    info("LUA %s", opts[ARG_lua_dir].string);
    info("times %i", opts[ARG_times].integer);
    info("reps %f", opts[ARG_repetitions].real);

    g.loop_events = vec_new();

    g.events = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        /* keys are static strings */
        NULL,
        /* vals are malloc'd vectors, destroy */
        events_destroy_val
    );

    for (int i = 0, l = (sizeof EVENTS) / sizeof EVENTS[0]; i < l; i++) {
        g_hash_table_insert(g.events, (char *) EVENTS[i], (gpointer) vec_new());
    }

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

    return true;
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

/* Print (bullet is U2022)
 * •...
 * .•..
 * ..•.
 * ...•
 * ..•.
 * .•..
 */
static void print_hold_indicator(short s) {
    static short DELAY = 2; // how many times to stay on the same pos
    static short NUM = 5;
    static short last_state = 0;
    static short t = -1;
    static short delay_cur = -1;
    if (!s) {
        t = -1;
        delay_cur = DELAY - 1;
        last_state = 0;
        printf("\n");
    }
    else {
        if (last_state == 0) {
            last_state = 1;
            printf("\n");
        }
        delay_cur = (delay_cur + 1) % (DELAY + 1);
        if (delay_cur == DELAY) 
            t = (t + 1) % (2*NUM - 2);
        if (t < NUM) {
            printf("\r[ ");
            for (int i = 0; i < NUM; i++) {
                printf("%s", i == t ? "•" : ".");
            }
            printf(" ]");
        }
        else {
            printf("\r[ ");
            for (int i = 0; i < NUM; i++) {
                int u = 2*(NUM - 1) - t;
                printf("%s", i == u ? "•" : ".");
            }
            printf(" ]");
        }
    }
}

static int add_listener_l(lua_State *L) {
    char *errs = NULL;
    const char *event = luaL_checkstring(L, -2);
    if (strcmp(lua_typename(L, lua_type(L, -1)), "function")) {
        errs = "add_listener_l(): need func at -1";
        goto ERR;
    }
    int reg_index = luaL_ref(L, LUA_REGISTRYINDEX); // also pops
    vec *lvec = (vec *) g_hash_table_lookup(g.events, event);
    if (! lvec) {
        _();
        BR(event);
        spr("No matching event for %s", _s);
        errs = _t;
    }
    vec_add(lvec, GINT_TO_POINTER(reg_index));
    ERR:
    if (errs) {
        lua_pushstring(L, errs);
        lua_error(L);
    }
    return 0;
}

bool main_fire_event(char *event, gpointer data) {
    vec *lvec = (vec *) g_hash_table_lookup(g.events, event);
    if (! lvec) {
        _();
        BR(event);
        iwarn("No matching event for %s", _s);
        return false;
    }
    for (int i = 0, l = vec_size(lvec); i < l; i++) {
        int args = 0;
        int reg_idx = GPOINTER_TO_INT(vec_get(lvec, i));
        lua_rawgeti(global.L, LUA_REGISTRYINDEX, reg_idx);
        if (! strcmp(event, "random")) {
            lua_pushboolean(global.L, (bool) GPOINTER_TO_INT(data));
            args = 1;
        }
        /* Don't set ok to false -- not serious enough. */
        int rc;
        if ((rc = lua_pcall(global.L, args, 0, 0))) {
            const char *err = luaL_checkstring(global.L, -1);
            _();
            BR(event);
            check_lua_err(rc, "Lua error on event %s: %s", _s, err);
            continue;
        }
    }
    return true;
}

/* Can't (usefully) fail */
void main_remove_timeout(guint id) {
    bool always_true = g_source_remove(id);
    (void) always_true;
}

/* Can't (usefully) fail */
guint main_add_timeout(int ms, gpointer timeout_func, gpointer data) {
    guint id = g_timeout_add(ms, (GSourceFunc) timeout_func, data);
    return id;
}

guint main_add_fd_watch(int fd, GIOCondition cond, gpointer func, gpointer data) {
    GIOChannel *channel = g_io_channel_unix_new(fd);
    guint id = g_io_add_watch(channel, cond, (GIOFunc) func, data);
    return id;
}

int config_l(lua_State *L) {
    int num_rules = (sizeof CONF) / (sizeof CONF[0]) - 1;

    if (! flua_config_load_config(g.conf, CONF, num_rules)) {
        _();
        BR("Couldn't load lua config.");
        lua_pushstring(L, _s);
        lua_error(L);
    }
    g.lua_initted = true;

    return 0;
}

static int add_timeout_l(lua_State *L) {
    int reg_index = luaL_ref(L, LUA_REGISTRYINDEX); // also pops
    bool repeat = lua_toboolean(L, -1);
    int ms = (int) luaL_checknumber(L, -2);
    guint id;
    if (! add_timeout_lua_func(ms, L, reg_index, repeat, &id)) {
        lua_pushstring(L, "Couldn't add user timeout.");
        lua_error(L);
    }
    lua_pushnumber(L, id);
    return 1;
}

static int remove_timeout_l(lua_State *L) {
    int id = (int) luaL_checknumber(L, -1);
    // can't fail
    main_remove_timeout(id);
    return 0;
}

