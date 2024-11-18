#ifndef _CONNECTIONPOOL_H_
#define _CONNECTIONPOOL_H_

#include "yasio/yasio.hpp"
#include <mutex>
#include <condition_variable>
#include <mysql/include/mysql.h>
#include <functional>
#include <thread>
#include <queue>
#include <future>

using namespace yasio;

typedef std::vector<std::vector<std::string>> call_back_data;
typedef std::function<void(call_back_data&)> call_func;
typedef std::string lua_code;

struct CallFuncStruct {
  call_func call_back;
  std::string sql;
  call_back_data data;
};

class ConnectionPool {
public:
  ConnectionPool(io_service* gservice);
  ~ConnectionPool();

  void query(const std::string sql, std::function<void(call_back_data)> row_cb_f);
  static void loadLuaCode(const std::string code);
  void setLuaInit(void* lua);

  void init();
  std::string host, user, pass, db = "";
  int port=0;

  io_service* service;

private:
};

#endif