#include <connectionPool.h>
#include <iostream>
#include <luainit.h>
extern "C" {
#include "log.h"
}

std::mutex mtx;
std::queue<CallFuncStruct> messages;
std::queue<CallFuncStruct> messages_main;
std::queue<lua_code> messages_lua;
std::condition_variable cv;
LuaInit* gcluainit;

bool getData(CallFuncStruct* d, const std::string& url, const std::string& user, const std::string& password, const std::string& db, int port)
{
  if (d->sql.empty())
    return false;
  std::unique_lock<std::mutex> lock(mtx);
  MYSQL* conn = mysql_init(nullptr);
  lock.unlock();
  mysql_real_connect(conn, url.c_str(), user.c_str(), password.c_str(), db.c_str(), port, nullptr, 0);
  if (conn == NULL)
  {
    printf("[mysql->error] failed to get connection\n");
    mysql_close(conn);
    conn = nullptr;
    return false;
  }
  std::string sql = d->sql;
  if (mysql_query(conn, sql.c_str()))
  {
    printf("[mysql->error] query failed: %s [sqlerror:%s] \n", mysql_error(conn), sql.c_str());
    mysql_close(conn);
    conn = nullptr;
    return false;
  }

  MYSQL_RES* res = mysql_store_result(conn);
  if (res == nullptr)
  {
    // printf("[mysql->error] query failed: %s\n", mysql_error(conn));
    mysql_close(conn);
    conn = nullptr;
    return false;
  }
  int num_fields = mysql_num_fields(res);

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(res)))
  {
    std::vector<std::string> list;
    for (int i = 0; i < num_fields; i++)
    {
      list.push_back(((std::string)row[i]));
    }
    d->data.push_back(list);
  }

  mysql_free_result(res);
  res = nullptr;
  row = nullptr;
  mysql_close(conn);
  conn = nullptr;
  return true;
}

ConnectionPool::ConnectionPool(io_service* service) : service(service) {}

void ConnectionPool::init()
{
  if (host.empty())
  {
    printf("数据库配置为null\n");
    return;
  }

  //// 不停循环有没有处理完后的数据，有就回调
  service->schedule(std::chrono::milliseconds(10), [](io_service& service) {
    while (true)
    {
      std::unique_lock<std::mutex> lock(mtx);
      if (messages_main.empty())
        return false;
      CallFuncStruct msg = messages_main.front(); // 获取消息
      messages_main.pop();                        // 从队列中移除消息
      lock.unlock();
      msg.call_back(msg.data);
    }
    return false;
  });
  service->schedule(std::chrono::milliseconds(500), [](io_service& service) {
    while (true)
    {
      std::unique_lock<std::mutex> lock(mtx);
      if (messages_lua.empty())
        return false;
      lua_code msg = messages_lua.front(); // 获取消息
      messages_lua.pop();                  // 从队列中移除消息
      lock.unlock();
      gcluainit->loadCode(msg);
    }
    return false;
  });
}

ConnectionPool::~ConnectionPool() {}

void ConnectionPool::query(const std::string sql, std::function<void(call_back_data)> row_cb_f)
{
  if (sql.empty())
    return;

  std::thread(
      [](const std::string& host, const std::string& user, const std::string& pass, const std::string& db, int port, const std::string& sql,
         std::function<void(call_back_data)> row_cb_f) {
        CallFuncStruct d; // 获取消息
        d.sql       = sql;
        d.call_back = row_cb_f;

        if (getData(&d, host, user, pass, db, port))
        {
          std::unique_lock<std::mutex> lock(mtx);
          messages_main.push(d); // 向队列中添加消息
        }
      },
      host, user, pass, db, port, sql, row_cb_f)
      .detach();
}

void ConnectionPool::loadLuaCode(const std::string code)
{
  if (code.empty())
    return;

  std::unique_lock<std::mutex> lock(mtx);
  messages_lua.push(code);
}

void ConnectionPool::setLuaInit(void* lua) { gcluainit = (LuaInit*)lua; }
