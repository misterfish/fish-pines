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

capi.mpd.config_func(config.mpd)
capi.nes.config_func(config.nes)

function rule (buttons, options, cb) 
    local buttons_as_numbers = map(
        function (i,v)
            return cbuttons[v]
        end, 
    buttons)

--    for i,v in ipairs(buttons_as_numbers) do
--        io.write(string.format("%d: %d", i, v))
--    end

end

rule({'b', 'right'}, { kill_multiple = false }, creak)
