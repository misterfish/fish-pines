needs ({ me = 'custom' }, '__imported_util')

local function switch_to_wireless_client_mode ()
    info 'Switching to internet wired'
    sys (configlua.cmds.switch_to_wireless_client_mode)
end

local function switch_to_ap_mode ()
    info 'Switching to ap mode'
    sys (configlua.cmds.switch_to_ap_mode)
end

return {
    switch_to_wireless = switch_to_wireless,
    switch_to_wired = switch_to_wired,
}
