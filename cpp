cpp $(pkg-config glib-2.0 --cflags) -I /usr/include/lua5.1 -Ifish-lib-util/fish-utils -Ifish-lib-util/fish-util "$1" |& less
