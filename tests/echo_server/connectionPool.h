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

class ConnectionPool {
public:
  ConnectionPool(const std::string& url, const std::string& user, const std::string& password, const std::string& db, int port, io_service* gservice);
  ~ConnectionPool();

  void query(const std::string sql, std::function<void(std::vector<std::string>)> row_cb_f);
  std::string url, user, password, db;
  int port;

  io_service* service;

private:
};

#endif