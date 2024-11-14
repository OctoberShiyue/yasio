#include <connectionPool.h>

ConnectionPool::ConnectionPool(const std::string& url, const std::string& user, const std::string& password, const std::string& db, int port, int maxSize)
    : url(url), user(user), password(password), db(db), port(port), maxSize(maxSize)
{
  for (int i = 0; i < maxSize; ++i)
  {
    connections.push(createConnection());
  }
}

ConnectionPool::~ConnectionPool()
{
  while (!connections.empty())
  {
    delete connections.front();
    connections.pop();
  }
}

MYSQL* ConnectionPool::getConnection()
{
  std::unique_lock<std::mutex> lock(mtx);
  cond.wait(lock, [this]() { return !connections.empty(); });

  MYSQL* con = connections.front();
  connections.pop();
  return con;
}

void ConnectionPool::releaseConnection(MYSQL* con)
{
  std::unique_lock<std::mutex> lock(mtx);
  connections.push(con);
  cond.notify_one();
}

void ConnectionPool::query(MYSQL_ROW& row, const std::string& sql)
{
  MYSQL* conn = getConnection();
  if (conn == NULL)
  {
    printf("[mysql->error] failed to get connection\n");
    return;
  }
  if (mysql_query(conn, sql.c_str()))
  {
    printf("[mysql->error] query failed: %s\n", mysql_error(conn));
    releaseConnection(conn);
    return;
  }
  MYSQL_RES* res = mysql_store_result(conn);
  if (res == NULL)
  {
    //printf("[mysql->error] failed: %s \n", mysql_error(conn));
    releaseConnection(conn);
    return;
  }
  row = mysql_fetch_row(res);
  mysql_free_result(res);
  releaseConnection(conn);
}
