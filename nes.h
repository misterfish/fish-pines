#include <wiringPi.h>
#include <piNes.h>

bool nes_init_config();
int nes_init();
int nes_read(int joystick);

int nes_config_l(lua_State *L);

char *nes_get_path_dpin();
char *nes_get_path_cpin();
char *nes_get_path_lpin();
