return {
    mpd = {
        seek = 5, -- secs
        volupamount = 2,
        voldownamount = 2,
    },
    shutdown = {
        secs = 1.3,
        cmd = 'sudo shutdown now',

        --secs = 4,
        --cmd = 'sudo ls',
    },
    cmds = {
        switch_to_wireless_client_mode = 'switch-to-wifi-client',
        switch_to_ap_mode    = 'switch-to-ap-mode',
        make_playlist_all = 'make-playlist-all',

        silence_out     = false,
    },
    leds = {
        -- BCM numbering.
        random = 6,
        update = 15,
        mode = 16,
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
