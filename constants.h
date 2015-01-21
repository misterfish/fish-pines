#ifndef __INCL_CONSTANTS_H
#define __INCL_CONSTANTS_H

/* From wiringPi.
 */
# define	N_RIGHT	0x01
# define	N_LEFT	0x02
# define	N_DOWN	0x04
# define	N_UP		0x08
# define	N_START	0x10
# define	N_SELECT	0x20
# define	N_B		0x40
# define	N_A		0x80

/* Our std order.
 */

#define F_LEFT      0
#define F_RIGHT     1
#define F_UP        2
#define F_DOWN      3
#define F_SELECT    4
#define F_START     5
#define F_B         6
#define F_A         7

#ifdef NO_NES
static int POLL_TENTHS_OF_A_SECOND = 1;
#else
static int POLL_MS = 40;
#endif

/* Poll mpd for updates every times through loop, i.e., if it's 5 and
 * POLL_MS is 40, every 200 ms
 */
static int MPD_UPDATE = 10; 

#define MPD_HOST "localhost"
#define MPD_PORT 6600 // 0 also ok
#define MPD_TIMEOUT_MS 3000
#define MPD_TIMEOUT_PLAYLIST_MS 30000
#define MPD_PLAY_ON_LOAD_PLAYLIST true

#define MODE_MUSIC      0x00
#define MODE_GENERAL    0x01

#endif
