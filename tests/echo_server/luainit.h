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
  LuaInit( ConnectionPool* mysql_pool, std::map<int, Player*>* Players);
  ~LuaInit();

  void notifyConnent(int connent_type, Player* p, DataPacket* packet);
  void setService(io_service* service);
  void luaregister(lua_State* L);
  void loadCode(std::string code);
  void init();

  std::string mysql_host, mysql_user, mysql_pass, mysql_db;
  int mysql_port;
  std::string service_ip = "0.0.0.0";
  u_short service_port  = 19999;

private:
  lua_State* L;
};

#endif