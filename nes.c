#include <fish-util.h>

#include "global.h"
#include "const.h"
#include "flua_config.h"

#include "nes.h"

#define CONF_NAMESPACE "nes"

extern short BCM2WIRINGPI(short);

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

int nes_config_l(lua_State *L) {
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

static bool init_wiringPi() {
    if (wiringPiSetup() == -1) {
        warn("Unable to call wiringPiSetup()");
        pieprf;
    }
    return true;
}

static int init_joystick() {
    int joystick;
    short dpin_bcm = conf_i(dpin);
    short cpin_bcm = conf_i(cpin);
    short lpin_bcm = conf_i(lpin);

    short dpin = BCM2WIRINGPI(dpin_bcm);
    short lpin = BCM2WIRINGPI(lpin_bcm);
    short cpin = BCM2WIRINGPI(cpin_bcm);
    if (! dpin) {
        _();
        spr("%d", dpin_bcm);
        BR(_s);
        warn("Can't convert dpin (%s) to wiringPi numbering.", _t);
        return -1;
    }
    if (! cpin) {
        _();
        spr("%d", cpin_bcm);
        BR(_s);
        warn("Can't convert cpin (%s) to wiringPi numbering.", _t);
        return -1;
    }
    if (! lpin) {
        _();
        spr("%d", lpin_bcm);
        BR(_s);
        warn("Can't convert lpin (%s) to wiringPi numbering.", _t);
        return -1;
    }

    if ((joystick = setupNesJoystick(dpin, cpin, lpin)) == -1) {
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
