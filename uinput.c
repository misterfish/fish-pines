#define _GNU_SOURCE 

/* This was for turning nes events into mouse/keyboard events for xmms; not
 * necessary for mpd.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
// memset
#include <string.h>
#include <time.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include <fish-util.h>
#include "uinput.h"

static const char* DEV = "/dev/uinput";
static const int USLEEP = 15000;

static struct {
    int uinput;
} g;

void uinput_init() {
    g.uinput = open(DEV, O_WRONLY | O_NONBLOCK);

    if(g.uinput < 0) {
        err("Couldn't open %s: %s", Y_(DEV), R_(perr()));
    }

    if (ioctl(g.uinput, UI_SET_EVBIT, EV_ABS)) 
        die_perr();
    if (ioctl(g.uinput, UI_SET_ABSBIT, ABS_X)) 
        die_perr();
    if (ioctl(g.uinput, UI_SET_ABSBIT, ABS_Y)) 
        die_perr();

    if (ioctl(g.uinput, UI_SET_EVBIT, EV_KEY)) 
        die_perr();
    if (ioctl(g.uinput, UI_SET_KEYBIT, BTN_0)) 
        die_perr();
    if (ioctl(g.uinput, UI_SET_KEYBIT, BTN_1)) 
        die_perr();
    if (ioctl(g.uinput, UI_SET_KEYBIT, BTN_2)) 
        die_perr();
    if (ioctl(g.uinput, UI_SET_KEYBIT, BTN_3)) 
        die_perr();

    // 'synchronization events', necessary?
    if (ioctl(g.uinput, UI_SET_EVBIT, EV_SYN))
        die_perr();

    struct uinput_user_dev uidev;

    memset(&uidev, 0, sizeof(uidev));

    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-sample");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0xfedc;
    uidev.id.version = 1;

    uidev.absmin[ABS_X] = 0;
    uidev.absmax[ABS_X] = 255;
    uidev.absmin[ABS_Y] = 0;
    uidev.absmax[ABS_Y] = 255;

    if( write(g.uinput, &uidev, sizeof(uidev)) <= 0)
        die_perr();

    if( ioctl(g.uinput, UI_DEV_CREATE) )
        die_perr();
}


void uinput_inject_dir(int axis, int val) {
    struct input_event ev;

    memset(&ev, 0, sizeof(ev));

    ev.type = EV_ABS;
    ev.code = axis;
    ev.value = val;

    int i;
    if( (i = write(g.uinput, &ev, sizeof(ev))) <= 0) 
        die_perr();

    //infof ("Wrote %d bytes", i);

    usleep(USLEEP);
}

void uinput_inject_key(int code, int val) {
    struct input_event ev;

    memset(&ev, 0, sizeof(ev));

    ev.type = EV_KEY;
    ev.code = code;
    ev.value = val;

    int i;
    if( (i = write(g.uinput, &ev, sizeof(ev))) <= 0) 
        die_perr();

    usleep(USLEEP);
}

void _delay() {
    usleep(USLEEP);
}

void uinput_center_x() {
    uinput_inject_dir(ABS_X, 128);
}
void uinput_center_y() {
    uinput_inject_dir(ABS_Y, 128);
}

void uinput_left() {
    uinput_inject_dir(ABS_X, 12);
}
void uinput_right() {
    uinput_inject_dir(ABS_X, 255);
}
void uinput_up() {
    uinput_inject_dir(ABS_Y, 12);
}
void uinput_down() {
    uinput_inject_dir(ABS_Y, 255);
}
void uinput_btn_select() {
    uinput_inject_key(BTN_0, 1);
    _delay();
    uinput_inject_key(BTN_0, 0);
}
void uinput_btn_start() {
    uinput_inject_key(BTN_1, 1);
    _delay();
    uinput_inject_key(BTN_1, 0);
}
void uinput_btn_b_down() {
    uinput_inject_key(BTN_2, 1);
}
void uinput_btn_b_up() {
    uinput_inject_key(BTN_2, 0);
}
void uinput_btn_a() {
    uinput_inject_key(BTN_3, 1);
    _delay();
    uinput_inject_key(BTN_3, 0);
}
