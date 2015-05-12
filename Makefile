# Use caps for vars which users are allowed to initialise from outside (and
# CC, which is special).

cc 		= gcc 
CC 		= $(cc) 

CFLAGS		+= -std=c99 
LDFLAGS		?= 

main		= fish-pines

# Will be looped over to build <module>_cflags, <module>_ldflags, etc.
modules 	= fishutil fishutils
submodules	= fish-lib-util

fishutil_dir		= fish-lib-util
fishutil_cflags		= $(shell PKG_CONFIG_PATH=$(fishutil_dir)/pkg-config/static pkg-config --cflags fish-util)
fishutil_ldflags	= $(shell PKG_CONFIG_PATH=$(fishutil_dir)/pkg-config/static pkg-config --static --libs fish-util)
fishutils_cflags	= $(shell PKG_CONFIG_PATH=$(fishutil_dir)/pkg-config/static pkg-config --cflags fish-utils)
fishutils_ldflags	= $(shell PKG_CONFIG_PATH=$(fishutil_dir)/pkg-config/static pkg-config --static --libs fish-utils)

ifeq ($(NO_NES), 1)
    CC 	+= -DNO_NES
endif

ifeq ($(DEBUG), 1)
    CC += -DDEBUG
endif

modules 		= mpdclient wiringPi fishutil fishutils
submodules		= fish-lib-util
pkg_names		=

mpdclient_cflags	= # system-wide
mpdclient_ldflags	= -lmpdclient 

wiringPi_cflags		= -IwiringPi/devLib -IwiringPi/wiringPi 
wiringPi_ldflags	=

ifneq ($(NO_NES), 1)
    wiringPi_ldflags	+= -LwiringPi/wiringPi -lwiringPi -LwiringPi/devLib -lwiringPiDev
endif

CFLAGS		+= -W -Wall -Wextra -I./
CFLAGS		+= $(foreach i,$(modules),$(${i}_cflags))

LDFLAGS		+= -Wl,--export-dynamic
LDFLAGS		+= $(foreach i,$(modules),$(${i}_ldflags))

src		= $(main).c vol.c mode.c buttons.c ctl-default.c ctl-custom.c led.c mpd.c util.c global.c

hdr		= vol.h mode.h buttons.h ctl-default.h ctl-custom.h led.h mpd.h util.h

obj		= $(main).o vol.o mode.o buttons.o ctl-default.o ctl-custom.o led.o mpd.o util.o global.o

ifneq ($(NO_NES), 1)
    src  	+= nes.c
    obj		+= nes.o
endif

all: submodules $(main)

submodules: 
	for i in "$(submodules)"; do \
	    cd "$$i"; \
	    make; \
	    cd ..; \
	done;

$(main): $(src) $(hdr) $(obj)
	@#echo $(CC) $(CFLAGS) $(LDFLAGS) $(obj) -o $(main)
	$(CC) $(CFLAGS) $(LDFLAGS) $(obj) -o $(main)

# Note that all objs get rebuilt if any header changes. 

$(obj): %.o: %.c $(hdr)
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -f *.o
	rm -f *.so
	rm -f $(main)
	cd $(fishutil_dir) && make clean

mrproper: clean
	cd $(fishutil_dir) && make mrproper

.PHONY: all clean mrproper
