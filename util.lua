needs ({ me = 'util' }, 'posix', 'capi')

-- import everything from module into the given namespace.
-- do it via metatable.
local function import (into, module)
    -- look up unknown names first with the new __index, then the old one.
    -- in other words 'unshift'
    local mt = getmetatable (into) or {}
    local index_old = mt.__index or function () end
    setmetatable (into, {__index = function (tbl, varname, val)
        if val == nil then
            local lookup = module[varname] or index_old (tbl, varname, val)
            return lookup
        end
    end})
end

local function sayf (format, ...)
    local i, v, tbl
    tbl = {}
    for i, v in ipairs { select (1, ...) } do
        tbl[i] = tostring (v)
    end
    format = format .. "\n"
    io.write (string.format (format, unpack (tbl)))
end

local function say (...)
    local i, v, tbl
    tbl = {}
    for i, v in ipairs { select (1, ...) } do
        tbl[i] = tostring (v)
    end
    io.write (join (' ', unpack (tbl)))
    io.write ('\n')
end

local function join (joiner, ...)
    local str = ''
    local v
    local first = true
    for _, v in ipairs { select (1, ...) } do
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
    return string.format (...)
end

local function printf (format, ...)
    if format == nil then
        io.write ("<nil>")
        return
    end
    io.write (string.format (format, ...))
end

local function bullet ()
    return '٭'
end

local function warnf (format, ...)
    sayf ("%s " .. format, BR (bullet ()), ...)
    return nil
end

local function warn (string)
    warnf (string)
    return nil
end

local function infof (format, ...)
    sayf ("%s " .. format, B (bullet ()), ...)
end

local function info (string)
    infof (string)
    return nil
end

-- step back n times in stack for error message.
-- 1 is the current function. so most library calls use error2, since 2
-- refers to the calling function.
-- but since error2 is itself a function add 1 more, and since they route
-- through errorn for generality, add yet 1 more.

local function errorn (n, str)
    str = string.format("[stack - %s]: ", Y(n - 1)) .. str
    error (str, n + 2)
end

local function errornf (n, format, ...)
    format = string.format("[stack - %s]: ", Y(n - 1)) .. format
    error (string.format (format, ...), n + 2)
end

local function error2 (str)
    errorn (2, str)
end

local function error2f (format, ...)
    errornf (2, format, ...)
end

local function error3 (str)
    errorn (3, str)
end

local function error3f (format, ...)
    errornf (3, format, ...)
end

local function ipush (tbl, ...)
    for i, v in select (1, ...) do
        tbl[#tbl + 1] = v
    end
    return tbl
end

local function push (tbl, ...)
    for _, v in ipairs { select (1, ...) } do
        tbl[table.maxn (tbl) + 1] = v
    end
    return tbl
end

local function table_concat (tbl1, tbl2)
    for _, v in ipairs (tbl2) do
        push (tbl1, v)
    end
end

-- takes itable as input, and map_fn (v, i)
-- caller is free to mess with i.
-- depending on that, the return is an itable or a table.
local function imap (map_fn, itable)
    local result = {}
    for i, v in ipairs (itable) do
        local newi, newv
        newi, newv = map_fn (v, i)
        result[newi] = newv
    end
    return result
end

local function sys (cmd, args)
    -- os.execute (cmd), like 'system'
    -- os.popen (cmd), pipe open. Doesn't exist in 5.1
    -- also os.execute gives different values depending on lua version.
    if _VERSION == "Lua 5.1" then -- deprecated
        local c = cmd .. "> /dev/null"
        local code = os.execute (c)
        if code ~= 0 then
            warnf ("Couldn't execute cmd «%s», code was «%s»", BR (cmd), Y (code))
        end
    --assume newer
    elseif os.popen then
        local fh = os.popen (cmd)
        if fh then
            if not fh:read ('*a') then
                -- even an invalid command should give a valid read
                error "bad read from os.popen"
            end
        else
            error "got nil filehandle from os.popen"
        end
        local ok, how, value = fh:close ()
        if not ok then
            local str = spr ("Couldn't execute cmd «%s»", BR (cmd))
            if how == 'signal' then
                str = str .. spr (", got signal «%s»", CY (value))
            elseif how == 'exit' then
                if value == 0 then
                else
                    str = str .. spr (", exit code was «%s»", CY (value))
                end
            end
            warn (str)
        end
    else
        os.execute(cmd)  -- XX
        return true
    end

end

-- takes table as input, and map_fn (v, k)
-- caller is free to mess with k.
-- depending on that, the return is an itable or a table.
-- actually map can do everything imap can do, only imap officially
-- calls ipairs instead of pairs.
local function map (map_fn, table)
    local result = {}
    for k, v in pairs (table) do
        local newk, newv
        newk, newv = map_fn (v, k)
        result[newk] = newv
    end
    return result
end

local function color (col, s)
    if (type (s) ~= "string") then
        s = tostring (s)
    end
    return string.format ('[' .. '%d' .. 'm' .. '%s' .. '[0m', col, s)
end

local function G (s) return color (32, s) end
local function BG (s) return color (92, s) end
local function Y (s) return color (33, s) end
local function BY (s) return color (93, s) end
local function R (s) return color (31, s) end
local function BR (s) return color (91, s) end
local function B (s) return color (34, s) end
local function BB (s) return color (94, s) end
local function M (s) return color (35, s) end
local function BM (s) return color (95, s) end
local function CY (s) return color (36, s) end
local function BCY (s) return color (96, s) end

local function fork_wait (cmd, args)
    if not cmd then error "fork_wait: missing cmd" end
    args = args or {}

    local onsuccess = args.onsuccess or nil
    local onwait = args.onwait or nil
    local onfailure = args.onfailure or nil

    local verbose_ok = args.verbose_ok or false
    local silence_out = args.silence_out or true

    local pid, errmsg = posix.fork ()
    if not pid then
        error ("fork_wait: can't fork: " .. errmsg)
    -- kindje
    elseif pid == 0 then
        if silence_out then
            util.close_stdout()
        end
        local ok, str, int = os.execute (cmd)
        local okstr
        if not ok then
            okstr = BR ('not ok')
        else
            okstr = G ('ok')
        end
        local msg
        if str == 'signal' then
            msg = spr ("killed by signal «%s»", BR (int))
        elseif str == 'exit' then
            if int ~= 0 then
                msg = spr ("exited with status «%s»", BR (int))
            else
                msg = nil
            end
        else
            error ("fork_wait: unexpected string: " .. str)
        end
        if ok and verbose_ok then
            infof ("Cmd done, %s.", G ('ok'))
        elseif not ok then
            infof ("Error with cmd «%s»: %s.", BR (cmd), msg)
        end
        posix._exit (int)
    -- papaatje
    else
        while true do
            local cpid, str, int = posix.wait (pid, posix.WNOHANG)
            if not cpid then
                local errmsg, errnum = str, int
                error ("Error on wait: " .. errmsg)
            elseif cpid == 0 then
                -- still waiting, call onwait () cb
                if onwait then
                    onwait ()
                end
                --coroutine.resume (flashco)
            else
                local how, status = str, int
                local msg
                if str == 'killed' then
                    msg = spr ("killed by signal «%s»", BR (int))
                elseif str == 'stopped' then
                    msg = spr ("stopped by signal «%s»", BR (int))
                elseif str == 'exited' then
                    if int ~= 0 then
                        msg = spr ("exited with status «%s»", BR (int))
                    else
                        msg = nil
                    end
                else
                    error ("fork_wait: unexpected string: " .. str)
                end
                local prnt = "Child " .. Y (cpid) .. " done, %s."
                --local okstr
                if int ~= 0 then
                    --okstr = BR 'not ok'
                    warnf (prnt, join (': ', BR 'not ok', msg))
                    -- make sure this is the last instruction in the
                    -- function.
                    if onfailure then
                        onfailure ()
                    end
                else
                    if verbose_ok then
                        infof (prnt, G 'ok')
                    end
                    -- make sure this is the last instruction in the
                    -- function.
                    if onsuccess then
                        onsuccess ()
                    end
                end
                break
            end -- end if wait results
            --coroutine.resume (flashco)
        end -- end wait loop
    end -- end papa
end

-- wrapper around fork_wait to create a coroutine for waiting on a command.
-- it has the same callback hooks as fork_wait, and on on_wait it yields.
-- the yields are meant to be read as:
--
-- local yield, how = coro.resume ()
--
-- see 'hashooks' in coro.pool for an example.

local function fork_wait_coro (args)
    args = args or error2 'fork_wait_coro: need args'
    local cmd = args.cmd or error2 'fork_wait_coro: need cmd'
    local verbose = args.verbose or false
    local verbose_ok = args.verbose_ok or false
    local silence_out = args.silence_out or false

    local coro = coroutine.create (function ()
        local isok = false
        util.fork_wait (cmd, {
            onsuccess = function ()
                -- we're done, don't yield, just return from the function.
                isok = true
            end,
            onwait = function ()
                -- on the other side:
                -- local yield, how = resume; how.ok will be nil.
                coroutine.yield {}
            end,
            onfailure = function ()
                -- same here.
                isok = false -- not nil
            end,
            verbose = verbose,
            verbose_ok = verbose_ok,
            silence_out = silence_out,
        })

        -- done waiting.
        -- on the other side:
        -- local yield, how = resume; how.ok will be true or false.
        return { ok = isok }
    end)

    return coro
end

-- requires capi.util.redirect_write_to_dev_null
-- use this to circumvent lua difficulties with closing stdout.
local function close_stdout ()
    capi.util.redirect_write_to_dev_null(posix.fileno(io.stdout))
end

return {
    __imported_util = true,

    import = import,
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
    error2 = error2,
    error2f = error2f,
    error3 = error3,
    error3f = error3f,
    ipush = ipush,
    push = push,
    table_concat = table_concat,
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
    fork_wait = fork_wait,
    fork_wait_coro = fork_wait_coro,
    close_stdout = close_stdout,
}
