#ifndef _LUAINIT_H_
#define _LUAINIT_H_

#include "yasio/yasio.hpp"
#include <signal.h>
#include <connectionPool.h>
#include <lua54/lua.hpp>
#include <player.h>

using namespace yasio;

class LuaInit {
public:
  LuaInit(io_service* service, ConnectionPool* mysql_pool, std::map<int, Player*>* Players);
  ~LuaInit();

  void luaregister(lua_State* L);

private:
  lua_State* L;
};

#endif