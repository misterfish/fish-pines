cc = gcc

fish_util_dir = fish-lib-util/fish-util
fish_utils_dir = fish-lib-util/fish-utils

fish_util_dep_flags = 	$(shell cat $(fish_util_dir)/dep_flags)

# XX: fish_util.o, fish_utils.o

fish_util_o = 		$(shell find $(fish_util_dir)/.obj -iname '*.o' | sed 's/\.obj\///')
fish_utils_dep_flags = 	$(shell cat $(fish_utils_dir)/dep_flags)
fish_utils_o = 		$(shell find $(fish_utils_dir)/.obj -iname '*.o' | sed 's/\.obj\///')

wiring_pi_flags = -IwiringPi/devLib -IwiringPi/wiringPi -LwiringPi/wiringPi -lwiringPi -LwiringPi/devLib -lwiringPiDev

fish_utils_all_flags = -I$(fish_util_dir) -I$(fish_utils_dir) $(fish_util_o) $(fish_utils_o) $(fish_util_dep_flags) $(fish_utils_dep_flags)

extra_libs = -lmpdclient -lpcre

all_lib_flags = $(fish_utils_all_flags) $(extra_libs) $(wiring_pi_flags)

cflags = -std=c99 $(all_lib_flags)

obj = ctl.o led.o mpd.o nes.o uinput.o util.o 

all: fish-pines

# fish-util deps always trigger a make XX
fish-pines: fish-util fish-utils $(obj)
	$(cc) $(cflags) $(obj) fish-pines.c -o fish-pines

ctl.o: ctl.c ctl.h
	$(cc) $(cflags) -c ctl.c -o ctl.o

led.o: led.c led.h
	$(cc) $(cflags) -c led.c -o led.o

mpd.o: mpd.c mpd.h
	$(cc) $(cflags) -c mpd.c -o mpd.o

nes.o: nes.c nes.h
	$(cc) $(cflags) $(wiring_pi_flags) -c nes.c -o nes.o

uinput.o: uinput.c uinput.h
	$(cc) $(cflags) -c uinput.c -o uinput.o

util.o: util.c util.h
	$(cc) $(cflags) -c util.c -o util.o

fish-util: 
	sh -c 'cd fish-lib-util/fish-util; make'

fish-utils:
	sh -c 'cd fish-lib-util/fish-utils; make'

clean: 
	rm -f *.o
	rm -f *.so
	sh -c 'cd fish-lib-util/fish-util; make clean'
	sh -c 'cd fish-lib-util/fish-utils; make clean'
	rm -f fish-pines
