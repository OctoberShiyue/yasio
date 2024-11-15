#include <player.h>
#include <iostream>

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

void Player::LoginSucces(std::function<void(bool&)> b_cb_f)
{
  auto uid = this->uid;
  // 更新在线时间
  mysql_pool->query("update `user` set `login_time`=" + std::to_string(login_time) + ",`online_time`=" + std::to_string(login_time) +
                        " where `uid`=" + std::to_string(uid),
                    [=](std::vector<std::string> data) {});

  printf("[client->%lld] %d login->uid %lld\n", getTimeStamp(), this->id, this->uid);

  auto th           = this->ib;
  auto thisplayer   = this;
  auto thmysql_pool = mysql_pool;

  this->heartbeat_time = service->schedule(std::chrono::milliseconds(10000), [th, thisplayer, thmysql_pool, uid](io_service& service) {
    auto now_time = getTimeStamp();

    // 更新在线时间
    thmysql_pool->query("update `user` set `online_time`=" + std::to_string(now_time / 1000) + " where `uid`=" + std::to_string(uid),
                        [=](std::vector<std::string> data) {});

    if (now_time - thisplayer->online_time <= 30000)
    {
      return false;
    }
    service.close((transport_handle_t)th);
    return true;
  });
  bool b               = true;
  b_cb_f(b);
}

void Player::Login(int64_t uid, std::string pass, std::function<void(bool&)> b_cb_f)
{
  this->uid = uid;
  this->onetime->cancel();
  this->onetime           = nullptr;
  this->login_time        = getTimeStamp() / 1000;
  this->online_time       = getTimeStamp() / 1000;
  this->pass              = pass;
  std::thread::id this_id = std::this_thread::get_id();

  mysql_pool->query("select pass from `user` where `uid`=" + std::to_string(uid), [=](std::vector<std::string> data) {
    std::thread::id this_id = std::this_thread::get_id();
    bool b                  = false;

    if (data.size() == 0)
    {
      // 创建玩家数据
      mysql_pool->query("INSERT INTO `user`(uid, reg_time, login_time, online_time, pass)VALUES(" + std::to_string(uid) + "," +
                            std::to_string(this->login_time) + "," + std::to_string(this->login_time) + "," + std::to_string(this->online_time) + ",'" +
                            pass.data() + "')",
                        [=](std::vector<std::string> data) { this->LoginSucces(b_cb_f); });
      return;
    }
    else if (data[0] != pass) // 密码不对，退出登录
    {
      b = false;
      b_cb_f(b);
      return;
    }
    this->LoginSucces(b_cb_f);
  });
}
