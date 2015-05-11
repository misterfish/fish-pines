fishutilx_topdir = fish-lib-util
fishutil_dir = $(fishutilx_topdir)/fish-util
fishutils_dir = $(fishutilx_topdir)/fish-utils

cc = gcc -std=c99

ifeq ($(NO_NES), 1)
    cc 	+= -DNO_NES
endif

ifeq ($(DEBUG), 1)
    cc += -DDEBUG
endif

modules = mpdclient wiringPi fishutil fishutils

# shared, system-wide install
mpdclient_all	:= -lmpdclient 

# shared, submodule.
wiringPi_inc	:= -IwiringPi/devLib -IwiringPi/wiringPi 
wiringPi_all	:= $(wiringPi_inc)
ifneq ($(NO_NES), 1)
    wiringPi_all	+= -LwiringPi/wiringPi -lwiringPi -LwiringPi/devLib -lwiringPiDev
endif

# static, submodule.

# sets <module>_inc, <module>_obj, <module>_src_dep, <module>_ld, and <module>_all.
include $(fishutil_dir)/fishutil.mk
include $(fishutils_dir)/fishutils.mk
VPATH=$(fishutil_dir) $(fishutils_dir)

inc		= $(foreach i,$(modules),$(${i}_inc))
all		= $(foreach i,$(modules),$(${i}_all))

pre		:= $(fishutil_obj) $(fishutils_obj)
main		:= fish-pines
src_c		:= vol.c mode.c buttons.c ctl-default.c ctl-custom.c led.c mpd.c uinput.c util.c
ifneq ($(NO_NES), 1)
    src_c	+= nes.c
endif
extra_headers	:= conf.h
src		:= $(src_c) \
    		    $(main).c $(main).h \
		    $(src_c:.c=.h)
obj		:= $(src_c:.c=.o)

all: $(pre) $(obj) $(main)

$(obj): %.o: %.c $(extra_headers)
	$(cc) $(inc) -c $< -o $@

$(main): $(fishutil_obj) $(fishutils_obj) $(obj) $(src)
	$(cc) $(all) $(main).c $(obj) -o $(main)

$(fishutil_obj): $(fishutil_src_dep)
	make -C $(fishutilx_topdir)

$(fishutils_obj): $(fishutils_src_dep)
	make -C $(fishutilx_topdir)

clean: 
	rm -f *.o
	rm -f *.so
	cd $(fishutilx_topdir) && make clean
	rm -f $(main)

mrproper: clean
	cd $(fishutilx_topdir) && make mrproper

.PHONY: all clean mrproper
