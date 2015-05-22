return {
    mpd = {
        seek = 5, -- secs
    },
    shutdown = {
        secs = 1.3,
        --cmd = 'sudo shutdown',
        cmd = 'sudo ls',
    },
    cmds = {
        switch_to_wireless = 'switch-to-internet-wireless',
        switch_to_wired    = 'switch-to-internet-wired',
    },
    leds = {
        -- BCM numbering.
        random = 15,
        mode = 18,
    }
}
