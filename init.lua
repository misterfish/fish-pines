local capi = capi
local cbuttons = capi.buttons
local cmpd = capi.mpd

require 'config'

function say (format, ...) 
    format = format .. '\n'
    io.write(string.format(format, ...))
end

function map (map_fn, itable)
    local result = {}
    local idx = 0
    for i,v in ipairs(itable) do
        idx = idx + 1
        result[idx] = map_fn(i, v)
    end
    return result
end

for _,v in pairs {'mpd', 'nes', 'mode' } do
--for _,v in pairs {'mpd', 'nes',  } do
    capi[v].config_func(config[v])
end

function stickthyme()
    say('stick thyme!')
end
function stickthyme2()
    say('oregano!')
end

local rules = {
    -- mode = music
    music = {
        press = {
            { 'b', 'right', kill_multiple = false, handler = stickthyme },
            { 'b',          kill_multiple = false, handler = stickthyme2 },
            { 'a', 'right', kill_multiple = false, handler = stickthyme2 },
            { 'a',          kill_multiple = true, handler = stickthyme2 },
        },
        release = {
            { 'b'       , handler = stickthyme3 },
        }
    },
    -- mode = general
    general = {
    }
}

for mode,t in pairs(rules) do
    for event,u in pairs(t) do
        for _,rule in ipairs(u) do
            rule.mode = mode
            rule.event = event
            capi.buttons.add_rule(rule)
        end
    end
end
