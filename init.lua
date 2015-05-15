local capi = capi

io.write("Starting script, Frank.\n")

--capi.creak()
io.write(string.format("%d", capi.buttons.up))
io.write(string.format("%d", capi.buttons.left))

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

function rule(buttons, options, cb) 
    local buttons_as_numbers = map(
        function (i,v)
            return capi.buttons[v]
        end, 
    buttons)

    for i,v in ipairs(buttons_as_numbers) do
        io.write(string.format("%d: %d", i, v))
    end

end

rule({'b', 'right'}, { kill_multiple = false }, creak)
