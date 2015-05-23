needs ({ me = 'shutdown' }, '__imported_util', 'capi', 'configlua')

local time_down = {
    secs = nil,
    usecs = nil,
}

local disabled = false

local function go () 
    info "Shutting down!"
    sys (configlua.shutdown.cmd)
    disabled = true
end

local function start_pressed () 
    if disabled then 
        return 
    end

    if not time_down.secs and not time_down.usecs then
        time_down.secs, time_down.usecs = capi.util.get_clock ()
        return
    end

    local toen = {}
    local now = {}
    now.secs, now.usecs = capi.util.get_clock ()
    toen.secs, toen.usecs = time_down.secs, time_down.usecs

    for _, v in pairs {now, toen} do
        -- v.secs is about 30 bits. Won't overflow double.
        v.combined = v.secs + v.usecs * 1e-6
    end

    if now.combined - toen.combined > configlua.shutdown.secs then
        go ()
    end
end

local function start_released ()
    time_down.secs = nil
    time_down.usecs = nil
end

return {
    start_pressed = start_pressed,
    start_released = start_released
}
