#include <player.h>

Player::Player(io_service* service, io_base* th, ConnectionPool* mysql_pool)
{
  this->ib         = th;
  this->id         = th->id();
  this->service    = service;
  this->mysql_pool = mysql_pool;
  printf("[client->%lld] %d connect\n", getTimeStamp(), this->id);

  this->onetime = service->schedule(std::chrono::milliseconds(3000), [th](io_service& service) {
    service.close((transport_handle_t)th);
    return true;
  });
}

Player::~Player()
{
  if (this->ib == nullptr)
    return;
  if (this->heartbeat_time != nullptr)
  {
    this->heartbeat_time->cancel();
  }
  this->heartbeat_time = nullptr;
  this->ib             = nullptr;
  this->onetime        = nullptr;

  printf("[client->%lld] %d disconnect\n", getTimeStamp(), this->id);
}

bool Player::Login(int64_t uid, std::string pass)
{
  this->uid = uid;
  this->onetime->cancel();
  this->onetime     = nullptr;
  this->login_time  = getTimeStamp() / 1000;
  this->online_time = getTimeStamp() / 1000;
  this->pass        = pass;

  MYSQL_ROW row;
  mysql_pool->query(row, "select pass from `user` where `uid`=" + std::to_string(uid));

  if (row == NULL)
  {
    // 创建玩家数据
    mysql_pool->query(row, "INSERT INTO `user`(uid, reg_time, login_time, online_time, pass)VALUES(" + std::to_string(uid) + "," +
                               std::to_string(this->login_time) + "," + std::to_string(this->login_time) + "," + std::to_string(this->online_time) + ",'" +
                               pass.data() + "')");
  }
  else if (((std::string)row[0]) != pass) // 密码不对，退出登录
  {
    return false;
  }
  else
  {
    // 更新在线时间
    mysql_pool->query(row, "update `user` set `login_time`=" + std::to_string(login_time) + ",`online_time`=" + std::to_string(login_time) +
                               " where `uid`=" + std::to_string(uid));
  }

  printf("[client->%lld] %d login->uid %lld\n", getTimeStamp(), this->id, this->uid);

  auto th           = this->ib;
  auto thisplayer   = this;
  auto thmysql_pool = mysql_pool;

  this->heartbeat_time = service->schedule(std::chrono::milliseconds(10000), [th, thisplayer, thmysql_pool, uid](io_service& service) {
    auto now_time = getTimeStamp();

    // 更新在线时间
    MYSQL_ROW row;
    thmysql_pool->query(row, "update `user` set `online_time`=" + std::to_string(now_time / 1000) + " where `uid`=" + std::to_string(uid));

    if (now_time - thisplayer->online_time <= 30000)
    {
      return false;
    }
    service.close((transport_handle_t)th);
    return true;
  });

  return true;
}
