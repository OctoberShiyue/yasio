#ifndef _LUAINIT_H_
#define _LUAINIT_H_

#include "yasio/yasio.hpp"
#include <signal.h>
#include <connectionPool.h>
#include <lua54/lua.hpp>
#include <player.h>
#include <datapacket.h>

using namespace yasio;

class LuaInit {
public:
  LuaInit(io_service* service, ConnectionPool* mysql_pool, std::map<int, Player*>* Players);
  ~LuaInit();
  std::string mysql_host, mysql_user, mysql_pass, mysql_db;
  int mysql_port;
  void notifyConnent(int connent_type, Player* p, DataPacket* packet);
  void updateMysqlInfo();

  void luaregister(lua_State* L);

private:
  lua_State* L;
};

#endif