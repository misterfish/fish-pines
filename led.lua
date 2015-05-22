local function on (which) 
    local pin = configlua.leds[which]
    if not pin then return warnf("Can't find led for %s", BR(which)) end
    capi.gpio.on(pin)
end

local function off (which) 
    local pin = configlua.leds[which]
    if not pin then return warnf("Can't find led for %s", BR(which)) end
    capi.gpio.off(pin)
end

local function get (which)
    local pin = configlua.leds[which]
    if not pin then return warnf("Can't find led for %s", BR(which)) end
    return capi.gpio.read(pin)
end

return {
    on = on,
    off = off,
    get = get
}
