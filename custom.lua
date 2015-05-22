local function switch_to_wireless () 
    info 'Switching to internet wired'
    util.sys(configlua.cmds.switch_to_wireless)
end

local function switch_to_wired () 
    info 'Switching to internet wired'
    util.sys(configlua.cmds.switch_to_wired)
end

return {
    switch_to_wireless = switch_to_wireless,
    switch_to_wired = switch_to_wired,
}
