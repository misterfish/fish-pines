-- checks for needed modules.
--
-- needs( [args], 'module1', 'module2', ... )
-- args ::= { [ me = '<string>' ], [ check = [_G] ] }

return function (args, ...)
    local module = '<unknown>'
    local check = _G
    local needs_list
    if type(args) == 'table' then
        module = args.me or module
        check = args.check or check
        needs_list = { select(1, ...) }
    else
        needs_list = { args, select(1, ...) }
    end
    local _, symbol
    for _, symbol in ipairs(needs_list) do
        if not check[symbol] then
            error(string.format("Module %s needs symbol '%s'", module, symbol))
        end
    end
end

