needs ({ me = 'custom' }, '__imported_util')

local function switch_to_wireless () 
    info 'Switching to internet wired'
    sys (configlua.cmds.switch_to_wireless)
end

local function switch_to_wired () 
    info 'Switching to internet wired'
    sys (configlua.cmds.switch_to_wired)
end

return {
    switch_to_wireless = switch_to_wireless,
    switch_to_wired = switch_to_wired,
}
