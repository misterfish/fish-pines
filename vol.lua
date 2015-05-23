local function vol (str)
    local response = capi.util.socket_unix_message (configlua.vol.sock, str)
    if response then
        if configlua.verbose.sockets then
            sayf ("Got response: «%s»", G (response))
        end
    else
        warn "Couldn't send message to socket"
    end
end

local function up (amount) 
    if not amount then amount = configlua.vol.upamount end
    return vol ('rel all all ' .. amount) -- .. ok with num
end

local function down (amount) 
    if not amount then amount = configlua.vol.downamount end
    return vol ('rel all all ' .. (-1 * amount)) -- .. ok with num
end

return {
    up = configlua.anton_mode and down or up,
    down = configlua.anton_mode and up or down,
}
