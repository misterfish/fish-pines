local function sayf (format, ...)
    local i, v, tbl
    tbl = {}
    for i, v in ipairs { select(1, ...) } do
        tbl[i] = tostring(v)
    end
    format = format .. "\n"
    io.write(string.format(format, unpack(tbl)))
end

local function say (...) 
    local i, v, tbl
    tbl = {}
    for i, v in ipairs { select(1, ...) } do
        tbl[i] = tostring(v)
    end
    io.write(join(' ', unpack(tbl)))
    io.write('\n')
end

local function join(joiner, ...) 
    local str = ''
    local v
    local first = true
    for _, v in ipairs { select(1, ...) } do
        if not first then
            str = str .. joiner
        else
            first = false
        end
        str = str .. v
    end
    return str
end

local function spr (...)
    return string.format(...)
end

local function printf (format, ...) 
    if format == nil then
        io.write("<nil>")
        return
    end
    io.write(string.format(format, ...))
end

--local BULLETS = {'꣐', '⩕', '٭', '᳅', '�', '�', '�', '�', '�'}
local BULLETS = {'꣐', '⩕', '٭', '᳅'}

local function bullet () 
    return BULLETS[1 + math.floor(math.random() * #BULLETS)]
end

local function warnf (format, ...) 
    sayf("%s " .. format, BR(bullet()), ...)
    return nil
end

local function warn (string) 
    warnf(string)
    return nil
end

local function infof (format, ...)
    sayf("%s " .. format, B(bullet()), ...)
end

local function info (string) 
    infof(string)
    return nil
end

local function ipush (tbl, ...)
    for i, v in select(1, ...) do
        tbl[#tbl + 1] = v
    end
    return tbl
end

local function push (tbl, ...)
    for _,v in ipairs { select(1, ...) } do
        tbl[table.maxn(tbl) + 1] = v
    end
    return tbl
end

-- takes itable as input, and map_fn(v, i)
-- caller is free to mess with i.
-- depending on that, the return is an itable or a table.
local function imap (map_fn, itable)
    local result = {}
    for i, v in ipairs(itable) do
        local newi, newv
        newi, newv = map_fn(v, i)
        result[newi] = newv
    end
    return result
end

local function sys (cmd, opts) 
    -- os.execute(cmd), like 'system'
    -- os.popen(cmd), pipe open. Doesn't exist in 5.1
    -- also os.execute gives different values depending on lua version.
    if _VERSION == "Lua 5.1" then
        local c = cmd .. "> /dev/null"
        local code = os.execute(c)
        if code ~= 0 then
            warnf("Couldn't execute cmd «%s», code was «%s»", BR(cmd), Y(code))
        end
    --assume newer
    else
        local fh = os.popen(cmd) 
        if fh then
            if not fh:read('*a') then
                print "bad read (shouldn't happen, also not on bad cmd)" --iwarn XX
            end
        else
            print "(shouldn't happen)" --iwarn XX
        end
        local ok, how, value = fh:close()
        if not ok then
            local str = spr("Couldn't execute cmd «%s»", BR(cmd))
            if how == 'signal' then
                str = str .. spr(", got signal «%s»", CY(value))
            elseif how == 'exit' then 
                if value == 0 then
                else
                    str = str .. spr(", exit code was «%s»", CY(value))
                end
            end
            warn(str)
        end
    end
end

-- takes table as input, and map_fn(v, k)
-- caller is free to mess with k.
-- depending on that, the return is an itable or a table.
-- actually map can do everything imap can do, only imap officially
-- calls ipairs instead of pairs.
local function map (map_fn, table)
    local result = {}
    for k, v in pairs(table) do
        local newk, newv
        newk, newv = map_fn(v, k)
        result[newk] = newv
    end
    return result
end

local function color (col, s) 
    if (type(s) ~= "string") then
        s = tostring(s)
    end
    return string.format('[' .. '%d' .. 'm' .. '%s' .. '[0m', col, s) 
end

local function G (s) return color(32, s) end 
local function BG (s) return color(92, s) end 
local function Y (s) return color(33, s) end 
local function BY (s) return color(93, s) end 
local function R (s) return color(31, s) end 
local function BR (s) return color(91, s) end 
local function B (s) return color(34, s) end 
local function BB (s) return color(94, s) end 
local function M (s) return color(35, s) end 
local function BM (s) return color(95, s) end 
local function CY (s) return color(36, s) end 
local function BCY (s) return color(96, s) end 

return {
    sayf = sayf,
    say = say, 
    join = join,
    spr = spr,
    printf = printf ,
    bullet = bullet,
    warnf = warnf,
    warn = warn,
    infof = infof,
    info = info,
    ipush = ipush,
    push = push,
    imap = imap,
    map = map,
    color = color,
    sys = sys,
    G = G,
    BG = BG,
    Y = Y,
    BY = BY,
    R = R,
    BR = BR,
    B = B,
    BB = BB,
    M = M,
    BM = BM,
    CY = CY,
    BCY = BCY,
}
