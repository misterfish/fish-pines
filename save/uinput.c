#define _GNU_SOURCE 

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

#include "util/util.h"
#include "uinput.h"

void init(struct uinput_user_dev*);

const char* DEV = "/dev/uinput";
const int USLEEP = 15000;

const int CODES[] = {
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_S,
    KEY_UP,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_DOWN
};

const int CODES_L = sizeof CODES / sizeof(int);

struct {
    int uinput;
} g;

void uinput_init() {
    g.uinput = open(DEV, O_WRONLY | O_NONBLOCK);

    if(g.uinput < 0) {
        errorf("Couldn't open %s: %s", Y_(DEV), R_(perr()));
        exit(1);
    }

    if (ioctl(g.uinput, UI_SET_EVBIT, EV_KEY)) 
        diee();

    // 'synchronization events', necessary?
    if (ioctl(g.uinput, UI_SET_EVBIT, EV_SYN))
        diee();

    for (int i = 0; i < CODES_L; i++) {
        if( ioctl(g.uinput, UI_SET_KEYBIT, CODES[i])) 
            diee();
    }

    struct uinput_user_dev uidev;

    memset(uidev, 0, sizeof(*uidev));

    snprintf(uidev->name, UINPUT_MAX_NAME_SIZE, "uinput-sample");
    uidev->id.bustype = BUS_USB;
    uidev->id.vendor  = 0x1234;
    uidev->id.product = 0xfedc;
    uidev->id.version = 1;

    if( write(g.uinput, &uidev, sizeof(uidev)) <= 0)
        diee();

    if( ioctl(g.uinput, UI_DEV_CREATE) )
        diee();
}

void uinput_inject(int c) {
    struct input_event ev;

    memset(&ev, 0, sizeof(ev));

    ev.type = EV_KEY;
    ev.code = c;
    ev.value = 1;

    int i;
    if( (i = write(g.uinput, &ev, sizeof(ev))) <= 0) 
        diee();

    infof ("Wrote %d bytes", i);

    usleep(USLEEP);
};
