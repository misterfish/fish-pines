needs ({ me = 'coro' }, '__imported_util')

local _, k, v

-- resume coroutines sequentially until they all finish.
--
-- hashooks means the yields give a 'how' object, which gives info about
-- whether the coro is waiting, done, or failed.

local pool
pool = (function ()
    local function new (args)
        local self = {
            verbose = false,

            coros = {},
            numtasks = 0,

            done = {},
            numdone = 0,
            amdone = false,
            done_cb = nil,
        }

        local tasks = args.tasks or error2 "pool.new: need tasks"
        self.verbose = args.verbose or false
        self.done_cb = args.done_cb or nil

        (function ()
            local taskidx = 0
            local task
            for _, task in pairs (tasks) do
                local coro = task[1]
                local kind = task.kind or 'function'
                local hashooks = task.hashooks or false

                self.numtasks = self.numtasks + 1
                taskidx = taskidx + 1

                self.coros [taskidx] = { coro = coro, hashooks = hashooks }
            end

            return coros, numtasks
        end) ()

        setmetatable(self, {
            __index = function (tbl, varname, val)
                if val == nil then
                    return pool[varname]
                end
            end
        })
        return self
    end

    local function once (self) 
        local idx, cowrap
        for idx, cowrap in ipairs (self.coros) do
            if self.done [idx] then goto nxt end
            local coro = cowrap.coro or error 'need coro.coro'
            local hashooks = cowrap.hashooks or false

            if coroutine.status (coro) ~= 'suspended' then
                self.done [idx] = true
                self.numdone = self.numdone + 1
                if self.numdone == self.numtasks then
                    self.amdone = true
                    if self.done_cb then
                        self.done_cb ()
                    end
                    break
                end
                goto nxt
            end

            local yield, how 
            if hashooks then
                yield, how = coroutine.resume (coro)
            else
                yield = coroutine.resume (coro)
            end

            if not yield then
                error "Error running coroutine."
            end

            if hashooks then
                if how.ok == nil then
                    if self.verbose then 
                        infof ("Task %s still spinning.", 
                        ( ({ CY, Y }) [idx % 2 + 1] ) (idx)
                        )
                    end
                elseif how.ok then
                    if self.verbose then
                        infof ("Task %s went home.", G (idx))
                    end
                else
                    if self.verbose then
                        infof ("Task %s went crazy.", BR (idx))
                    end
                end
            end

            ::nxt::
        end
    end

    local function isdone (self)
        return self.amdone
    end

    return {
        new = new,
        once = once,
        isdone = isdone,
    }
end)()

    --if done_cb then done_cb () end

return {
    pool = pool,
}
