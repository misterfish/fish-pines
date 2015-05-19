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

        --the_thing = 30,
    },
    mode = {
        modes = {
            'music',
            'general',
        },
        fun = {
            1,2,3,
        },
        funt = {
            true, false, true,
        },
        funr = {
            1.2, 2.3, 109.2
        }
    }
}
