local capi = capi

require 'config'

function say (...) 
    io.write(...)
    io.write('\n')
end

function sayf (format, ...) 
    if format == nil then
        return
    end
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

function color(col, s) return string.format('[' .. '%d' .. 'm' .. '%s' .. '[0m', col, s) end
function G(s) return color(32, s) end 
function BG(s) return color(92, s) end 
function Y(s) return color(33, s) end 
function BY(s) return color(93, s) end 
function R(s) return color(31, s) end 
function BR(s) return color(91, s) end 
function B(s) return color(34, s) end 
function BB(s) return color(94, s) end 
function M(s) return color(35, s) end 
function BM(s) return color(95, s) end 
function CY(s) return color(36, s) end 
function BCY(s) return color(96, s) end 

function mode_next() 
    capi.mode.next_mode()
    local m = capi.mode.get_mode_name()
--[
    local col
    if m == config.mode.modes[1] then
        col = Y
    else
        col = CY
    end

    sayf("Switched to mode %s", col(m))
--]]
end

local rules = {
    -- mode = music
    music = {
        press = {
            { 'b', 'right', kill_multiple = false, handler = stickthyme },
            { 'b',          kill_multiple = false, handler = stickthyme2 },
            { 'a', 'right', kill_multiple = false, handler = stickthyme2 },
            { 'a',          kill_multiple = true, handler = stickthyme2 },
            { 'select',          kill_multiple = true, handler = mode_next  },
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
