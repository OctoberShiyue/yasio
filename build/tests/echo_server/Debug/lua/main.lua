
print("配置服务器端口：",serviceInit("0.0.0.0", 18199))
print("配置数据库：",mysqlInit("127.0.0.1", "root", "Aa1023261581", "testorpg", 3306))

require "lib.init"

-- 登录
SERVER_LOGIN = 1
-- 心跳
SERVER_HEARTBEAT = 2
-- 登录成功
SERVER_LOGIN_SUCCESS = 3

-- createTime(5*1000,function ()
--     print(sendClientData(2,SERVER_LOGIN_SUCCESS,json.encode({
--         value1=663,
--     })))
-- end)


---服务器发送过来的数据
---@param connent_type any 通道ID
---@param pid any //玩家ID
---@param uid any //玩家UID
---@param packet any //数据包
service_mssage = function(connent_type, pid, uid, packet)
    print(connent_type, pid, uid, packet)
    if connent_type == SERVER_LOGIN then
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
print("测试[mysqlQuery]",mysqlQuery("select * from `user` where `uid`=123456788",function ( d )
    print(d)
    for k,v in pairs(d) do
	print(k,v)
    end
end
))
