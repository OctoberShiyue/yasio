require "lib.init"

-- 登录
SERVER_LOGIN = 1
-- 心跳
SERVER_HEARTBEAT = 2
-- 登录成功
SERVER_LOGIN_SUCCESS = 3

--[[

    服务端出问题解决方案：
            -小bug，可以使用 -lua print("热更新代码")  来热更新
            -大bug，配置新端并换端口，把玩家引入到新端上

]]

--- 创建计时器，时间/回调函数，返回true结束/false，一直循环
-- createTime(5*1000,function ()
--     print(sendClientData(2,SERVER_LOGIN_SUCCESS,json.encode({
--         value1=663,
--     })))
-- end)

---服务器发送过来的数据
---@param connent_type number 通道ID
---@param pid number //玩家ID
---@param uid number //玩家UID
---@param packet string //数据包
service_mssage = function(connent_type, pid, uid, packet)
    print(connent_type, pid, uid, packet)
    if connent_type == SERVER_LOGIN then
        ---给客户端发送数据
        ---玩家PID，通道，json编译好的字符串数据
        sendClientData(
            pid,
            SERVER_LOGIN_SUCCESS,
            json.encode(
                {
                    value1 = 663
                }
            )
        )
    end
end

---返回的数据都是字符串列表的形式
print("测试[mysqlQuery]", mysqlQuery("select * from `user` where `uid`=123456788", function(d)
    print(d, #d)
    for k, v in pairs(d) do
        print(k, v)
        for k2, v2 in pairs(v) do
            print(k, v, k2, v2)
        end
    end
end))

gint=100

createTime(5*1000,function ()
   print("gint=",gint)
end)