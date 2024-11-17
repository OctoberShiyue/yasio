#include <connectionPool.h>
#include <iostream>
extern "C" {
#include "log.h"
}

typedef std::function<void(std::vector<std::string>&)> call_func;

struct CallFuncStruct {
  call_func call_back;
  std::string sql;
  std::vector<std::string> data;
};

std::mutex mtx;
std::queue<CallFuncStruct> messages;
std::queue<CallFuncStruct> messages_main;
std::condition_variable cv;

bool getData(CallFuncStruct* d, const std::string& url, const std::string& user, const std::string& password, const std::string& db, int port)
{
  if (d->sql.empty())
    return false;
  //// 开始连接数据库
  MYSQL* conn = mysql_init(nullptr);
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
    printf("[mysql->error] query failed: %s = %s \n", mysql_error(conn), sql.c_str());
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

  MYSQL_ROW row;
  // 获取字段数量
  int num_fields = mysql_num_fields(res);
  row            = mysql_fetch_row(res);
  if (row != NULL)
  {
    for (int i = 0; i < num_fields; i++)
    {

      d->data.push_back(((std::string)row[i]));
    }
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
  // 创建线程不停队列处理sql请求
  std::thread(
      [](const std::string& host, const std::string& user, const std::string& pass, const std::string& db, int port) {
        while (true)
        {
          std::unique_lock<std::mutex> lock(mtx);
          cv.wait(lock, [] { return !messages.empty(); });

          CallFuncStruct d = messages.front(); // 获取消息
          messages.pop();                      // 从队列中移除消息
          if (getData(&d, host, user, pass, db, port))
          {
            messages_main.push(d); // 向队列中添加消息
          }
          lock.unlock(); // 解锁以允许其他线程访问队列
        }
      },
      host, user, pass, db, port)
      .detach();

  // 不停循环有没有处理完后的数据，有就回调
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
}

ConnectionPool::~ConnectionPool() {}

void ConnectionPool::query(const std::string sql, std::function<void(std::vector<std::string>)> row_cb_f)
{
  if (sql.empty())
    return;
  // std::lock_guard<std::mutex> lock(mtx);
  CallFuncStruct cd{};
  cd.sql       = sql;
  cd.call_back = row_cb_f;
  messages.push(cd);
  cv.notify_one();
}
