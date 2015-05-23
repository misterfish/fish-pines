needs ({ me = 'coro' }, '__imported_util')

local _, k, v

-- resume coroutines sequentially until they all finish.
-- useful to also give one which is the master, for example to update a
-- busy/wait signal, sleep a bit, and poll the rest again.
--
-- hashooks means the yields give a 'how' object, which gives info about
-- whether the coro is waiting, done, or failed.

local function pool (args) 
    local tasks = args.tasks or 
        error "flash: need tasks"
    local verbose = args.verbose or false
    local done_cb = args.done or nil

    local cos, master, numtasks = (function ()
        local master; local numtasks = 0; local cos = {}

        local taskidx = 0
        local task
        for _, task in pairs (tasks) do
            local co = task[1]
            local kind = task.kind or 'function'
            local hashooks = task.hashooks or false
            if hashooks and task.master then
                error "hashooks and master are mutually exclusive"
            end

            if task.master then
                master = co
            else
                numtasks = numtasks + 1
                taskidx = taskidx + 1

                cos[taskidx] = { co = co, hashooks = hashooks }
            end
        end

        return cos, master, numtasks
    end) ()

    local done = {}
    local numdone = 0
    local idx, cowrap
    while true do
        if master then
            -- no hooks, just bare resume/yield.
            local yield = coroutine.resume (master)
            if not yield then
                error "Error running coroutine."
            end
        end

        for idx, cowrap in ipairs (cos) do
            if done[idx] then goto nxt end
            local co = cowrap.co or error 'need co.co'
            local hashooks = cowrap.hashooks or false

            if coroutine.status (co) ~= 'suspended' then
                done[idx] = true
                numdone = numdone + 1
                if numdone == numtasks then
                    goto done
                end
                goto nxt
            end

            local yield, how 
            if hashooks then
                yield, how = coroutine.resume (co)
            else
                yield = coroutine.resume (co)
            end

            if not yield then
                error "Error running coroutine."
            end

            if hashooks then
                if how.ok == nil then
                    if verbose then 
                        infof ("Task %s still spinning.", 
                        ( ({ CY, Y }) [idx % 2 + 1] ) (idx)
                        )
                    end
                elseif how.ok then
                    if verbose then
                        infof ("Task %s went home.", G (idx))
                    end
                else
                    if verbose then
                        infof ("Task %s went crazy.", BR (idx))
                    end
                end
            end

            ::nxt::
        end
    end
    ::done::

    if done_cb then done_cb () end
end

return {
    pool = pool,
}
