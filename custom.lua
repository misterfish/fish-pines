needs ({ me = 'custom' }, '__imported_util')

local function switch_to_wireless_client_mode ()
    info 'Switching to internet wired'
    sys (configlua.cmds.switch_to_wireless_client_mode)
end

local function switch_to_ap_mode ()
    info 'Switching to ap mode'
    sys (configlua.cmds.switch_to_ap_mode)
end

local function kill_us ()
    info 'Killing us'
--sys 's /etc/init.d/mpd restart'
    sys 's ka fish-pines'
end

local function sndi ()
    info 'sndi'
    sys 'sndi'
    kill_us ()
end

local function sndx ()
    info 'sndx'
    sys 'sndx'
    kill_us ()
end

local function reload ()
    sys 'mpc update'
    sys 'make-playlist-all'
end

local rules_replace = nil

local rules_enhance = {
    music = {
        press = {
            { 'start',         once = true, handler = function ()
                sys 'killerr ka mplayer'
            end },
        },
        release = {
        },
    },
    general = {
        press = {
            { 'b',          once = true, handler = switch_to_ap_mode },
            { 'a',          once = true, handler = switch_to_wireless_client_mode },
            { 'down',       once = true, handler = kill_us },
            { 'left',       once = true, handler = sndi },
            { 'right',      once = true, handler = sndx },
            { 'up',         once = true, handler = reload },
        },
        release = {
        },
    },
}

return {
    rules_replace = rules_replace,
    rules_enhance = rules_enhance,
}
