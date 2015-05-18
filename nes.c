#include <fish-util.h>

#include "global.h"
#include "const.h"
#include "flua_config.h"

#include "nes.h"

#define CONF_NAMESPACE "nes"

#define conf_s(x) \
    flua_config_get_string(g.conf, #x)
#define conf_b(x) \
    flua_config_get_boolean(g.conf, #x)
#define conf_r(x) \
    flua_config_get_real(g.conf, #x)
#define conf_i(x) \
    flua_config_get_integer(g.conf, #x)

static struct flua_config_conf_item_t CONF[] = {
    flua_conf_required(dpin, integer)
    flua_conf_required(cpin, integer)
    flua_conf_required(lpin, integer)
    flua_conf_last
};

static struct {
    struct flua_config_conf_t *conf;
    bool lua_initted;
} g;

bool nes_init_config() {
    g.conf = flua_config_new(global.L);
    if (!g.conf)
        pieprf;
    flua_config_set_namespace(g.conf, CONF_NAMESPACE);
    return true;
}

void nes_configl() {
    int num_rules = (sizeof CONF) / (sizeof CONF[0]) - 1;
    /* Throws.
     */
    if (! flua_config_load_config(g.conf, CONF, num_rules)) {
        _();
        BR("Couldn't load lua config.");
        lua_pushstring(global.L, _s);
        lua_error(global.L);
    }
    g.lua_initted = true;
}

static bool init_wiringPi() {
    if (wiringPiSetup() == -1) {
        warn("Unable to call wiringPiSetup()");
        pieprf;
    }
    return true;
}

static int init_joystick() {
    int joystick;
    if ((joystick = setupNesJoystick(conf_i(dpin), conf_i(cpin), conf_i(lpin))) == -1) {
        warn("Unable to set up joystick");
        pieprneg1;
    }

    return joystick;
}

int nes_init() {
    if (! g.lua_initted) {
        warn("%s: forgot lua init?", CONF_NAMESPACE);
        return -1;
    }
    if (! init_wiringPi()) 
        return -1;
    return init_joystick();
}


int nes_read(int joystick) {
    return readNesJoystick(joystick);
}
