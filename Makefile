# Use caps for vars which users are allowed to initialise from outside (and
# CC, which is special).

# fish-util remake should trigger a remake of us XX

cc 		= gcc -ggdb
CC 		= $(cc) 

ifeq ($(NO_NES), 1)
    CC 	+= -DNO_NES
endif

ifeq ($(DEBUG), 1)
    CC += -DDEBUG
endif

CFLAGS		+= -std=c99 
LDFLAGS		?= 

main		= fish-pines

# Will be looped over to put <module>_cflags, <module>_ldflags into CFLAGS
# and LDFLAGS.
modules_manual 		= fishutil fishutils wiringPi
modules_pkgconfig	= libmpdclient lua5.1 glib-2.0
# Subdirectories (will be make -C'ed).
# wiringPi
submodules		= fish-lib-util

fishutil_dir		= fish-lib-util
fishutil_cflags		= $(shell PKG_CONFIG_PATH=$(fishutil_dir)/pkg-config/static pkg-config --cflags fish-util)
fishutil_ldflags	= $(shell PKG_CONFIG_PATH=$(fishutil_dir)/pkg-config/static pkg-config --static --libs fish-util)
fishutils_cflags	= $(shell PKG_CONFIG_PATH=$(fishutil_dir)/pkg-config/static pkg-config --cflags fish-utils)
fishutils_ldflags	= $(shell PKG_CONFIG_PATH=$(fishutil_dir)/pkg-config/static pkg-config --static --libs fish-utils)

wiringPi_cflags		= -IwiringPi/devLib -IwiringPi/wiringPi 
wiringPi_ldflags	=

ifneq ($(NO_NES), 1)
    wiringPi_ldflags	+= -LwiringPi/wiringPi -lwiringPi -LwiringPi/devLib -lwiringPiDev
endif

CFLAGS		+= -Werror=implicit-function-declaration -W -Wall -Wextra -I./
CFLAGS		+= -Wno-missing-field-initializers # GCC bug with {0}
CFLAGS		+= $(foreach i,$(modules_manual),$(${i}_cflags))
CFLAGS		+= $(foreach i,$(modules_pkgconfig),$(shell pkg-config "$i" --cflags))

LDFLAGS		+= -Wl,--export-dynamic
LDFLAGS		+= $(foreach i,$(modules_manual),$(${i}_ldflags))
LDFLAGS		+= $(foreach i,$(modules_pkgconfig),$(shell pkg-config "$i" --libs))


src		= $(main).c vol.c buttons.c mpd.c gpio.c mode.c util.c flua_config.c

hdr		= vol.h buttons.h mpd.h gpio.h mode.h util.h global.h const.h flua_config.h

obj		= $(main).o vol.o buttons.o mpd.o gpio.o mode.o util.o flua_config.o

ifneq ($(NO_NES), 1)
    src  	+= nes.c
    obj		+= nes.o
endif

all: submodules $(main)

submodules: 
	@for i in "$(submodules)"; do \
	    make -C "$$i"; \
	done

$(main): $(src) $(hdr) $(obj)
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
