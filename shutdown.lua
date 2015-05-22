local time_down = {
    secs = nil,
    usecs = nil,
}

local function go () 
    say "Shutting down!"
    util.sys(configlua.shutdown.cmd)
end

local function start_pressed () 
    time_down.secs, time_down.usecs = capi.util.get_clock()
end

local function start_released ()
    if time_down.secs == -1 or time_down.usecs == -1 then
        return warn "start_released called before start_pressed."
    end
    local toen = {}
    local now = {}
    now.secs, now.usecs = capi.util.get_clock()
    toen.secs, toen.usecs = time_down.secs, time_down.usecs

    for _,v in pairs {now, toen} do
        -- v.secs is about 30 bits. Won't overflow double.
        v.combined = v.secs + v.usecs * 1e-6
    end

    if now.combined - toen.combined > configlua.shutdown.secs then
        go()
    else 
        time_down.secs = nil
        time_down.usecs = nil
    end
end


return {
    start_pressed = start_pressed,
    start_released = start_released
}
