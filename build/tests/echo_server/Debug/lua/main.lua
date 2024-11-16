require 'lib.init'

-- print(
--     createTime(
--         1000,
--         function()
--             print(1111)
--             return false
--         end
--     )
-- )

-- print(
--     createTime(
--         2000,
--         function()
--             print(2222)
--             return false
--         end
--     )
-- )
-- local i=50
-- createTime(
--     0,
--     function()
--         i=i-1
--         print(i)

--         return i==0
--     end
-- )
SERVER_LOGIN_SUCCESS=3

createTime(5*1000,function ()
    print(sendClientData(2,SERVER_LOGIN_SUCCESS,json.encode({
        value1=663,
    })))
end)