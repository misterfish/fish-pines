#define _GNU_SOURCE

#include <stdlib.h>

#include <fish-util.h>
#include <fish-asound.h>

#include "const.h"
#include "global.h"
#include "flua_config.h"
#include "util.h"
#include "vol.h"

#define CONF_NAMESPACE "vol"

static struct flua_config_conf_item_t CONF[] = {
    flua_conf_default(verbose, boolean, false)
    flua_conf_default(fasound_verbose, boolean, false)

    flua_conf_last
};

static struct {
    struct flua_config_conf_t *conf;
    bool lua_initted;

    const char *card_names_string[FASOUND_MAX_SOUND_CARDS];
    const char *card_names_hw[FASOUND_MAX_SOUND_CARDS];
    const char *ctl_names[FASOUND_MAX_SOUND_CARDS][FASOUND_MAX_ELEMS];
    int fds[FASOUND_MAX_SOUND_CARDS][FASOUND_MAX_FDS];
} g;

bool vol_init_config() {
    g.conf = flua_config_new(global.L);
    if (!g.conf)
        pieprf;
    flua_config_set_namespace(g.conf, CONF_NAMESPACE);
    return true;
}

int vol_config_l(lua_State *L) {
    int num_rules = (sizeof CONF) / (sizeof CONF[0]) - 1;

fprintf(stderr, "trying 10, %d rules", num_rules);
    /* Throws. 
     */
    if (! flua_config_load_config(g.conf, CONF, num_rules)) {
fprintf(stderr, "trying 20");
        _();
        BR("Couldn't load lua config.");
        lua_pushstring(L, _s);
        lua_error(L);
    }
fprintf(stderr, "trying 30");
    g.lua_initted = true;
fprintf(stderr, "trying 40");

    return 0;
}


bool vol_init() {
    if (! g.lua_initted) {
        warn("%s: forgot lua init?", CONF_NAMESPACE);
        return false;
    }

    int options = 0;
    if (! conf_b(fasound_verbose)) options |= FASOUND_OPTIONS_QUIET;
    if (! fasound_init(options, g.card_names_string, g.card_names_hw, g.ctl_names, g.fds)) {
        warn("Couldn't initialise fasound");
        return false;
    }

    for (int i = 0; i < FASOUND_MAX_SOUND_CARDS; i++) {
        const char *name = g.card_names_string[i];
        if (!name) 
            continue;
        info("Got card: %s", name);
    }

    return true;
}
bool vol_down() {
    return true;
}

bool vol_up() {
    return true;
}

#if 0  // fish vol version
static struct {
    char *fish_vol_sock;
} g;

bool vol_init() {
    /* This will end up sending a message to a random socket if someone
     * messes with our env. 
     * But if they can do we have bigger problems.
     */
    char *home = /*secure_*/getenv("HOME");
    int s1 = strnlen(home, 50);
    int s2 = strnlen(FISH_VOL_SOCK, 200);
    if (s2 == 200) {
        warn("FISH_VOL_SOCK too long");
        return false;
    }
    if (s1 == 50) {
        warn("ENV{HOME} too long");
        return false;
    }
    g.fish_vol_sock = f_malloc(sizeof(char) * (s1 + s2));
    sprintf(g.fish_vol_sock, FISH_VOL_SOCK, home);
    return true;
}

bool vol_up() {
    if (!f_socket_unix_message(g.fish_vol_sock, "rel all all +5")) 
        if (!f_socket_unix_message(VOLD_SOCK, "up")) 
            pieprf;
    return true;
}

bool vol_down() {
    if (!f_socket_unix_message(g.fish_vol_sock, "rel all all -5")) 
        if (!f_socket_unix_message(VOLD_SOCK, "down")) 
            pieprf;
    return true;
}
#endif
