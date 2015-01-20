#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h> //usleep
#include <assert.h>
#include <pthread.h>
#include <fish-util.h>

#include "constants.h"
#include "conf.h"
#include "buttons.h"
#include "ctl.h"

struct {
    pthread_t thread;
    bool thread_playlist_stop;
} g;

bool led_init() {
    g.thread_playlist_stop = false;
    return true;
}

// not thread-safe apparently XX
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

// not thread-safe apparently XX
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
    if (mode == MODE_MUSIC) {
        return _led_off(LED_MODE);
    }
    else if (mode == MODE_GENERAL) {
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

/*
void thread_playlist_flash() {
    bool on = false;
    while (! g.thread_playlist_stop) {
        // not thread-safe XX
        //on ? _led_on(LED_REMAKE_PLAYLIST) : _led_off(LED_REMAKE_PLAYLIST);
        on = ! on;
        usleep(.5e6);
    }
    printf("threaddone, what's the biggy\n");
}
*/

/*
// threads are causing crazy crashes XX
bool led_update_remake_playlist_start() {
return false;
    g.thread_playlist_stop = false;

    pthread_create(&g.thread, NULL, (void*)thread_playlist_flash, NULL);
}
bool led_update_remake_playlist_stop() {
    g.thread_playlist_stop = true;
}
*/
