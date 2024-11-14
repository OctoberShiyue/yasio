#ifndef _CONNECTIONPOOL_H_
#define _CONNECTIONPOOL_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <mysql/include/mysql.h>

class ConnectionPool {
public:
  ConnectionPool(const std::string& url, const std::string& user, const std::string& password, const std::string& db, int port, int maxSize);
  ~ConnectionPool();

  MYSQL* getConnection();

  void releaseConnection(MYSQL* con);

  void query(MYSQL_ROW &row,const std::string& sql);

private:
  MYSQL* createConnection()
  {
    MYSQL* mysql = mysql_init(nullptr);
    return mysql_real_connect(mysql, url.c_str(), user.c_str(), password.c_str(), db.c_str(), port, nullptr, 0);
  }

  std::queue<MYSQL*> connections;
  std::mutex mtx;
  std::condition_variable cond;
  std::string url, user, password,db;
  int port;
  int maxSize;
};

#endif