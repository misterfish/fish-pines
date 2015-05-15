local capi = capi
local cbuttons = capi.buttons
local cmpd = capi.mpd

require 'config'

function say (format, ...) 
    format = format .. '\n'
    io.write(string.format(format, ...))
end

say("Starting script, Frank.\n")
say("config sez allen = %d", config.mpd.port)

capi.mpd.config_func()

--capi.creak()
say("%d", cbuttons.up)
say("%d", cbuttons.left)

function creak()
    io.write("CREAK!")
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
