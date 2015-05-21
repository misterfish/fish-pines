--require 'io'
--require 'string'
--require 'math'

local table = table
local print = print
local select = select
local unpack = unpack
local ipairs = ipairs
local pairs = pairs
local type = type
local tostring = tostring
local io = io
local math = math
local string = string

module('util')

function say (...) 
    io.write(...)
    io.write('\n')
end

function spr (...)
    return string.format(...)
end

function printf (format, ...) 
    if format == nil then
        io.write("<nil>")
        return
    end
    io.write(string.format(format, ...))
end

function sayf (format, ...)
    return printf(format .. "\n", ...)
end

--local BULLETS = {'꣐', '⩕', '٭', '᳅', '�', '�', '�', '�', '�'}
local BULLETS = {'꣐', '⩕', '٭', '᳅'}

function bullet() 
    return BULLETS[1 + math.floor(math.random() * #BULLETS)]
end

function warnf (format, ...) 
    sayf("%s " .. format, BR(bullet()), ...)
    return nil
end

function warn (string) 
    warnf(string)
    return nil
end

function ipush (tbl, ...)
    for i, v in select(1, ...) do
        tbl[#tbl + 1] = v
    end
    return tbl
end

function push(tbl, ...)
    for _,v in ipairs { select(1, ...) } do
        tbl[table.maxn(tbl) + 1] = v
    end
    return tbl
end

-- takes itable as input, and map_fn(v, i)
-- caller is free to mess with i.
-- depending on that, the return is an itable or a table.
function imap (map_fn, itable)
    local result = {}
    for i, v in ipairs(itable) do
        local newi, newv
        newi, newv = map_fn(v, i)
        result[newi] = newv
    end
    return result
end

-- takes table as input, and map_fn(v, k)
-- caller is free to mess with k.
-- depending on that, the return is an itable or a table.
-- actually map can do everything imap can do, only imap officially
-- calls ipairs instead of pairs.
function map (map_fn, table)
    local result = {}
    for k, v in pairs(table) do
        local newk, newv
        newk, newv = map_fn(v, k)
        result[newk] = newv
    end
    return result
end

local function color(col, s) 
    if (type(s) ~= "string") then
        s = tostring(s)
    end
    return string.format('[' .. '%d' .. 'm' .. '%s' .. '[0m', col, s) 
end

function G(s) return color(32, s) end 
function BG(s) return color(92, s) end 
function Y(s) return color(33, s) end 
function BY(s) return color(93, s) end 
function R(s) return color(31, s) end 
function BR(s) return color(91, s) end 
function B(s) return color(34, s) end 
function BB(s) return color(94, s) end 
function M(s) return color(35, s) end 
function BM(s) return color(95, s) end 
function CY(s) return color(36, s) end 
function BCY(s) return color(96, s) end 





--[[ lua 5.2
        local ok, how, value = os.execute(cmd)
        local isok = true
        --if not ok then
        local str = spr("Couldn't execute cmd «%s»", BR(cmd))
        say(how)
        if how == 'signal' then
            isok = false
            str = str .. spr(", got signal «%s»", CY(value))
        elseif how == 'exit' then 
            if value == 0 then
                isok = true
            else
                isok = false
                str = str .. spr(", exit code was «%s»", CY(value))
            end
        end
]]
