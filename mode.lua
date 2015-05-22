local mode_names = imap(function (v, i) 
    return v, i
end, config.mode.modes)

local function idx_for_name(name) 
    local idx = mode_names[name]
    if not idx then
        warn("XXX") -- XX
    end
    return idx
end

local function next_mode() 
    capi.mode.next_mode()
    local mode_name = capi.mode.get_mode_name()
    local mode_idx = idx_for_name(mode_name)
    if not mode_idx then
        return warn("XXX") -- XX
    end
    if mode_idx == 1 then
        col = Y
        led.off('mode')
    elseif mode_idx == 2 then
        col = CY
        led.on('mode')
    end
end

return {
    next_mode = next_mode,
    idx_for_name = idx_for_name,
}
