#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h> //usleep
#include <assert.h>
#include <fish-util.h>

#include "constants.h"
#include "conf.h"
#include "buttons.h"
#include "mode.h"

bool led_init() {
    return true;
}

bool _led_on(int pin) {
    assert(pin < 100);
    assert(pin > 0);
    char *msg = str(5);
    sprintf(msg, "%d 1", pin);
    bool err = false;
    if (!socket_unix_message(LEDD_SOCK, msg)) {
        _();
        spr("%d", pin);
        Y(_s);
        warn("Couldn't turn on led %s", _t);
        err = true;
    }
    free(msg);
    return !err;
}

bool _led_off(int pin) {
    assert(pin < 100);
    assert(pin > 0);
    char *msg = str(5);
    sprintf(msg, "%d 0", pin);
    bool err = false;
    if (!socket_unix_message(LEDD_SOCK, msg)) {
        _();
        spr("%d", pin);
        Y(_s);
        warn("Couldn't turn off led %s", _t);
        err = true;
    }
    free(msg);
    return !err;
}

bool led_update_mode(int mode) {
    if (mode_music()) {
        return _led_off(LED_MODE);
    }
    else if (mode_general()) {
        return _led_on(LED_MODE);
    }
    else {
        pieprf;
    }
}

bool led_update_random(bool random) {
    if (random) 
        return _led_on(LED_RANDOM);
    else
        return _led_off(LED_RANDOM);
}
