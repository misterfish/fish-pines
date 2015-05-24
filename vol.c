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

#define VOL_ALL -1

#define else_chan_name_to_idx(chan_idx, str) \
    else if (! strcmp(str, "front left")) \
        chan_idx = 

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

    int num_cards;
    int num_elems[FASOUND_MAX_FDS];
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

    /* Throws. 
     */
    if (! flua_config_load_config(g.conf, CONF, num_rules)) {
        _();
        BR("Couldn't load lua config.");
        lua_pushstring(L, _s);
        lua_error(L);
    }
    g.lua_initted = true;

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
        g.num_cards++;
        //info("Got card: %s", name);
        for (int j = 0; j < FASOUND_MAX_ELEMS; j++) {
            if (! g.ctl_names[i][j]) 
                continue;
            g.num_elems[i]++;
        }
    }

    return true;
}

/* All the ints can have the special value VOL_ALL */
bool vol_rel(int card, int ctl, int chan, int delta_perc) {
    int card_floor, card_ceiling;

    if (card == VOL_ALL) {
        card_floor = 0;
        card_ceiling = g.num_cards - 1;
    }
    else
        card_floor = card_ceiling = card;

    int chan_api;
    if (chan == VOL_ALL)
        chan_api = FASOUND_CHAN_ALL;
    else
        chan_api = chan;

    for (int i = card_floor; i <= card_ceiling; i++) {
        int ctl_floor, ctl_ceiling;
        if (ctl == VOL_ALL) {
            ctl_floor = 0;
            ctl_ceiling = g.num_elems[i] - 1;
        }
        else
            ctl_floor = ctl_ceiling = ctl;

        for (int j = ctl_floor; j <= ctl_ceiling; j++) {
            if (! fasound_set_rel(i, j, chan_api, delta_perc) || ! fasound_update(i, j, NULL)) {
                _();
                spr("%d", i);
                spr("%d", j);
                if (chan == VOL_ALL)
                    spr("all");
                else
                    spr("%d", chan);
                spr("%d", delta_perc);
                BR(_s);
                BR(_t);
                BR(_u);
                BR(_v);
                warn("vol_rel(): couldn't set card %s ctl %s chan %s -> %s", _w, _x, _y, _z);
                continue;
            }
        }
    }
    return true;
}

static int get_chan_idx_for_name(const char *name);

int vol_rel_l(lua_State *L) {
    int delta_perc = (int) luaL_checknumber(L, -1);
    int chan_idx, ctl_idx, card_idx;
    const char *errs = NULL;
    if (! strcmp(lua_typename(L, lua_type(L, -2)), "string")) {
        const char *str = luaL_checkstring(L, -2);
        if (! strcmp(str, "all")) 
            chan_idx = VOL_ALL;
        else if (-1 != (chan_idx = get_chan_idx_for_name(str))) {
            // ok
        }
        else {
            errs = str;
            goto ERR;
        }
    }
    else 
        chan_idx = (int) luaL_checknumber(L, -2);

    if (! strcmp(lua_typename(L, lua_type(L, -3)), "string")) {
        const char *str = luaL_checkstring(L, -3);
        if (! strcmp(str, "all")) 
            ctl_idx = VOL_ALL;
        else {
            errs = str;
            goto ERR;
        }
    }
    else 
        ctl_idx = (int) luaL_checknumber(L, -3);

    if (! strcmp(lua_typename(L, lua_type(L, -4)), "string")) {
        const char *str = luaL_checkstring(L, -4);
        if (! strcmp(str, "all")) 
            card_idx = VOL_ALL;
        else {
            errs = str;
            goto ERR;
        }
    }
    else 
        card_idx = (int) luaL_checknumber(L, -4);

    if (! vol_rel(card_idx, ctl_idx, chan_idx, delta_perc)) {
        lua_pushstring(L, "Couldn't set volume.");
        lua_error(L);
    }

    ERR:
    if (errs) {
        _();
        BR(errs);
        spr("vol_rel_l: invalid string: %s", _s);
        lua_pushstring(L, _t);
        lua_error(L);
    }

    return 0;
}

static int get_chan_idx_for_name(const char *name) {
    /*
    if (! strcmp(name, "unknown"))
        return SND_MIXER_SCHN_UNKNOWN;
    else*/if (! strcmp(name, "front left"))
        return SND_MIXER_SCHN_FRONT_LEFT;
    else if (! strcmp(name, "front right"))
        return SND_MIXER_SCHN_FRONT_RIGHT;
    else if (! strcmp(name, "rear left"))
        return SND_MIXER_SCHN_REAR_LEFT;
    else if (! strcmp(name, "rear right"))
        return SND_MIXER_SCHN_REAR_RIGHT;
    else if (! strcmp(name, "front center"))
        return SND_MIXER_SCHN_FRONT_CENTER;
    else if (! strcmp(name, "woofer"))
        return SND_MIXER_SCHN_WOOFER;
    else if (! strcmp(name, "side left"))
        return SND_MIXER_SCHN_SIDE_LEFT;
    else if (! strcmp(name, "side right"))
        return SND_MIXER_SCHN_SIDE_RIGHT;
    else if (! strcmp(name, "rear center"))
        return SND_MIXER_SCHN_REAR_CENTER;
    else if (! strcmp(name, "mono"))
        return SND_MIXER_SCHN_MONO;
    else
        return -1;
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
