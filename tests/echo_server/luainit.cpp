#include <luainit.h>

io_service* gLuaService        = nullptr;
ConnectionPool* gLuamysql_pool = nullptr;
std::map<int, Player*>* gLuaPlayers;
std::string mysql_host2, mysql_user2, mysql_pass2, mysql_db2;
int mysql_port2;

std::string service_ip2;
u_short service_port2;

LuaInit::LuaInit(ConnectionPool* mysql_pool, std::map<int, Player*>* Players)
{
  gLuaPlayers    = Players;
  gLuamysql_pool = mysql_pool;
  L              = luaL_newstate();
  luaL_openlibs(L);
  luaregister(L);
  const char* luaCode = "require 'main'";
  if (luaL_dostring(L, luaCode))
  {
    fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
  }
}

LuaInit::~LuaInit() {}

void LuaInit::notifyConnent(int connent_type, Player* p, DataPacket* packet)
{
  lua_getglobal(L, "service_mssage");
  if (!lua_isfunction(L, -1))
  {
    lua_pop(L, 1); // 移除非函数值
    return;
  }
  // 压入参数
  lua_pushinteger(L, connent_type); // 通道ID
  lua_pushinteger(L, p->GetId());   // 玩家ID
  lua_pushinteger(L, p->GetUid());  // 玩家UID
  if (packet != nullptr)
  {
    lua_pushstring(L, packet->data.dump().c_str());
  }
  else
  {
    lua_pushstring(L, "{}");
  }

  if (lua_pcall(L, 4, 0, 0) != LUA_OK)
  {
    const char* error = lua_tostring(L, -1);
    fprintf(stderr, "Error calling service_message: %s\n", error);
    lua_pop(L, 1);
  }
}
void LuaInit::updateMysqlInfo()
{
  this->mysql_host = mysql_host2;
  this->mysql_user = mysql_user2;
  this->mysql_pass = mysql_pass2;
  this->mysql_db   = mysql_db2;
  this->mysql_port = mysql_port2;
}
void LuaInit::updateServiceInfo()
{

  this->service_ip   = service_ip2;
  this->service_port = service_port2;
}
void LuaInit::setService(io_service* service) { gLuaService = service; }
extern "C" {

static int createTime(lua_State* L)
{
  if (!lua_isnumber(L, 1) || !lua_isfunction(L, 2))
  {
    lua_pushboolean(L, false);
    return 1;
  }
  int64_t time = (int64_t)lua_tonumber(L, 1);
  lua_pushvalue(L, 2);                          // 将函数复制到栈顶
  int funcRef = luaL_ref(L, LUA_REGISTRYINDEX); // 将函数引用存储在注册表中

  gLuaService->schedule(std::chrono::milliseconds(time), [L, funcRef](io_service& service) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef); // 从注册表中获取函数
    if (lua_pcall(L, 0, 1, 0) != LUA_OK)
    {
      fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
      lua_pop(L, 1);
      return true;
    }
    bool result_b = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return result_b;
  });

  lua_pushboolean(L, true);
  return 1;
}

static int sendClientData(lua_State* L)
{
  // 玩家PID/通道数/json数据
  if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isstring(L, 3))
  {
    lua_pushboolean(L, false);
    return 1;
  }

  int pid = (int)lua_tonumber(L, 1);
  int id  = (int)lua_tonumber(L, 2);

  Player* p = (*gLuaPlayers)[pid];

  if (p == nullptr)
  {
    lua_pushboolean(L, false);
    return 1;
  }

  DataPacket pd{};

  pd.id                = id;
  nlohmann::json njson = nlohmann::json::parse(lua_tostring(L, 3));
  njson["pw"]          = p->pass.data();
  pd.data              = njson;

  auto obs = cxx14::make_unique<yasio::obstream>();
  pd.setObstream(obs.get());
  gLuaService->write(p->GetTransport(), obs->data(), obs->length());
  obs->clear();
  obs = nullptr;

  lua_pushboolean(L, true);
  return 1;
}

static int mysqlQuery(lua_State* L)
{
  if (!lua_isstring(L, 1) || !lua_isfunction(L, 2))
  {
    lua_pushboolean(L, false);
    return 1;
  }
  lua_pushvalue(L, 2);
  int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
  gLuamysql_pool->query(lua_tostring(L, 1), [=](std::vector<std::string> data) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef);
    lua_newtable(L);
    for (size_t i = 0; i < data.size(); i++)
    {
      lua_pushinteger(L, i + 1); // 压入值
      lua_pushstring(L, data[i].data());
      lua_settable(L, -3);
    }
    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
    {
      fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));

      return;
    }
    return;
  });

  lua_pushboolean(L, true);
  return 1;
}
}

static int mysqlInit(lua_State* L)
{

  if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L, 3) || !lua_isstring(L, 4) || !lua_isnumber(L, 5))
  {
    lua_pushboolean(L, false);
    return 1;
  }

  mysql_host2 = lua_tostring(L, 1);
  mysql_user2 = lua_tostring(L, 2);
  mysql_pass2 = lua_tostring(L, 3);
  mysql_db2   = lua_tostring(L, 4);
  mysql_port2 = (int)lua_tonumber(L, 5);

  lua_pushboolean(L, true);
  return 1;
}

static int serviceInit(lua_State* L)
{

  if (!lua_isstring(L, 1) || !lua_isnumber(L, 2))
  {
    lua_pushboolean(L, false);
    return 1;
  }

  service_ip2   = lua_tostring(L, 1);
  service_port2 = (int)lua_tonumber(L, 2);

  lua_pushboolean(L, true);
  return 1;
}
extern "C" {
void LuaInit::luaregister(lua_State* L)
{
  lua_pushcfunction(L, createTime);
  lua_setglobal(L, "createTime");

  lua_pushcfunction(L, sendClientData);
  lua_setglobal(L, "sendClientData");

  lua_pushcfunction(L, mysqlQuery);
  lua_setglobal(L, "mysqlQuery");

  lua_pushcfunction(L, mysqlInit);
  lua_setglobal(L, "mysqlInit");

  lua_pushcfunction(L, serviceInit);
  lua_setglobal(L, "serviceInit");

  // 加载路径
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");
  const char* current_path = lua_tostring(L, -1);
  lua_pop(L, 1);
  std::string new_path = std::string(current_path) + ";./scripts/?.lua";
  lua_pushstring(L, new_path.c_str());
  lua_setfield(L, -2, "path");
  lua_pop(L, 1);
}
}