local capi = capi

require 'config'
require 'util'

local u = util

local say = util.say
local printf = util.printf
local sayf = util.sayf
local imap = util.imap
local map = util.map

for _,v in pairs {'mpd', 'nes', 'mode' } do
    capi[v].config(config[v])
end

function mode_next() 
    capi.mode.next_mode()
    local m = capi.mode.get_mode_name()
    local col
    if m == config.mode.modes[1] then
        col = u.Y
    else
        col = u.CY
    end

    sayf("Switched to mode %s", col(m))
end

function toggle_random() 
    local rand = capi.mpd.toggle_random() 
    printf("Random set to %s", u.CY(rand))
end

local rules = {
    -- mode = music
    music = {
        press = {
            { 'b', 'right', handler = function() capi.mpd.seek(configlocal.mpd.seek) end },
            { 'b', 'left',  handler = function() capi.mpd.seek(-1 * configlocal.mpd.seek) end },
            { 'a',          once = true, handler = toggle_random },
            { 'select',     once = true, handler = mode_next  },
            { 'start',      once = true, handler = function() capi.mpd.toggle_play() end },
        },
        release = {
        }
    },
    -- mode = general
    general = {
        press = {
            { 'select',     once = true, handler = mode_next  },
        }
    }
}

-- called by C after init.
function buttons_config() 
    local mode_names = imap(function (v, i) 
        return v, i
    end, config.mode.modes)

    for mode,t in pairs(rules) do
        for event,u in pairs(t) do
            for _,rule in ipairs(u) do
                rule.mode = mode_names[mode] - 1
                rule.event = event
                capi.buttons.add_rule(rule)
            end
        end
    end
end
