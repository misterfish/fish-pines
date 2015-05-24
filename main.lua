needs ({ me = 'main' }, '__imported_util', 'capi')

local function add_timeout (args) 
    args = args or error2 'need args'
    local ms = args.ms or error2 'need ms'
    local repeating = args.repeating or false
    local func = args.func or error2 'need func'
    local id = capi.main.add_timeout (ms, repeating, func)
    return id
end

local function remove_timeout (args) 
    args = args or error2 'need args'
    local id = args.id or error2 'need id'
    capi.main.remove_timeout (id)
end

return {
    add_timeout = add_timeout,
    remove_timeout = remove_timeout,
}
