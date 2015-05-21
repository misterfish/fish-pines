config = {
    nes = {
        -- Using Broadcom gpio numbering.
        dpin = 25, --yellow
        cpin = 24, --red
        lpin = 23, --orange
    },
    mpd = {
        host = 'localhost',
        port = 6600,
        timeout_ms = 3000,
        timeout_playlist_ms = 3000,
        play_on_load_playlist = true,
        playlist_path = "mpd-playlists", -- relative to root
        -- update mpd every n times through the main loop.
        update_on_n_ticks = 10,
    },
    mode = {
        modes = {
            'music',
            'general',
        },
    }
}

configlocal = {
    mpd = {
        seek = 5, -- secs
    },
    shutdown_secs = 1.3,
}

--[[
#define LED_ORANGE 12
#define LED_BLUE 10

#define LED_MODE LED_ORANGE
#define LED_RANDOM LED_BLUE
#define LED_REMAKE_PLAYLIST LED_BLUE
]]
