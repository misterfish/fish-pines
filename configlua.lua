return {
    mpd = {
        seek = 5, -- secs
    },
    shutdown = {
        secs = 1.3,
        cmd = 'sudo shutdown now',

        --secs = 4,
        --cmd = 'sudo ls',
    },
    cmds = {
        switch_to_wireless = 'switch-to-internet-wireless',
        switch_to_wired    = 'switch-to-internet-wired',
        make_playlist_all = 'make-playlist-all',

        silence_out     = false,
    },
    leds = {
        -- BCM numbering.
        random = 15,
        update = 15,
        mode = 18,
        flash = {
            -- yield every <sleep> secs, flash every n times
            sleep = { secs = 0, msecs = 200 },
            ntimes = 3,
        },
    },
    vol = {
        upamount = 2,
        downamount = 2,
    },
    verbose = {
        sockets = false,
    },

    -- switch down and up for changing the volume.
    -- useful for bugging the bartender.
    anton_mode = true,

    default_playlist = 'all', -- all.m3u

    update_playlist_verbose_tasks = false,
    coroutine_pools_verbose_ok = false,
}
