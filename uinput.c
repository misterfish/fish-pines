#define _GNU_SOURCE 

/* This can be used for turning NES controller events into mouse/keyboard events, for example for a GUI.
 * Not currently being used.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include <fish-util.h>
#include "uinput.h"

static const char *DEV = "/dev/uinput";
static const int USLEEP = 15000;

static struct {
    int uinput;
} g;

#define wprf do { \
    warn_perr(""); \
    return false; \
} while (0); 

bool uinput_init() {
    g.uinput = open(DEV, O_WRONLY | O_NONBLOCK);

    if (g.uinput < 0) {
        _();
        Y(DEV);
        warn_perr("Couldn't open %s", _s);
        return false;
    }

    if (ioctl(g.uinput, UI_SET_EVBIT, EV_ABS)) 
        wprf;
    if (ioctl(g.uinput, UI_SET_ABSBIT, ABS_X)) 
        wprf;
    if (ioctl(g.uinput, UI_SET_ABSBIT, ABS_Y)) 
        wprf;

    if (ioctl(g.uinput, UI_SET_EVBIT, EV_KEY)) 
        wprf;
    if (ioctl(g.uinput, UI_SET_KEYBIT, BTN_0)) 
        wprf;
    if (ioctl(g.uinput, UI_SET_KEYBIT, BTN_1)) 
        wprf;
    if (ioctl(g.uinput, UI_SET_KEYBIT, BTN_2)) 
        wprf;
    if (ioctl(g.uinput, UI_SET_KEYBIT, BTN_3)) 
        wprf;

    // 'synchronization events', necessary?
    if (ioctl(g.uinput, UI_SET_EVBIT, EV_SYN))
        wprf;

    struct uinput_user_dev uidev = {0};

    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-sample");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0xfedc;
    uidev.id.version = 1;

    uidev.absmin[ABS_X] = 0;
    uidev.absmax[ABS_X] = 255;
    uidev.absmin[ABS_Y] = 0;
    uidev.absmax[ABS_Y] = 255;

    if (write(g.uinput, &uidev, sizeof(uidev)) <= 0)
        wprf;

    if (ioctl(g.uinput, UI_DEV_CREATE) )
        wprf;
}

static bool uinput_inject_dir(int axis, int val) {
    struct input_event ev = {0};

    ev.type = EV_ABS;
    ev.code = axis;
    ev.value = val;

    int i;
    if ((i = write(g.uinput, &ev, sizeof(ev))) <= 0) 
        wprf;

    usleep(USLEEP);
}

static bool uinput_inject_key(int code, int val) {
    struct input_event ev = {0};

    ev.type = EV_KEY;
    ev.code = code;
    ev.value = val;

    int i;
    if ((i = write(g.uinput, &ev, sizeof(ev))) <= 0) 
        wprf;

    usleep(USLEEP);
}

static void _delay() {
    usleep(USLEEP);
}

bool uinput_center_x() {
    return uinput_inject_dir(ABS_X, 128);
}
bool uinput_center_y() {
    return uinput_inject_dir(ABS_Y, 128);
}
bool uinput_left() {
    return uinput_inject_dir(ABS_X, 12);
}
bool uinput_right() {
    return uinput_inject_dir(ABS_X, 255);
}
bool uinput_up() {
    return uinput_inject_dir(ABS_Y, 12);
}
bool uinput_down() {
    return uinput_inject_dir(ABS_Y, 255);
}
bool uinput_btn_select() {
    if (!uinput_inject_key(BTN_0, 1))
        return false;
    _delay();
    return uinput_inject_key(BTN_0, 0);
}
bool uinput_btn_start() {
    if (!uinput_inject_key(BTN_1, 1)) 
        return false;
    _delay();
    return uinput_inject_key(BTN_1, 0);
}
bool uinput_btn_b_down() {
    return uinput_inject_key(BTN_2, 1);
}
bool uinput_btn_b_up() {
    return uinput_inject_key(BTN_2, 0);
}
bool uinput_btn_a() {
    if (!uinput_inject_key(BTN_3, 1)) 
        return false;
    _delay();
    return uinput_inject_key(BTN_3, 0);
}
