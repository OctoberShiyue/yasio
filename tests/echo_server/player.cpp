#include <player.h>
#include <iostream>

Player::Player(io_service* service, io_base* th, ConnectionPool* mysql_pool, unsigned int id)
{
  this->ib         = th;
  this->id         = id;
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
  if (this->onetime!=nullptr)
  {
    this->onetime->cancel();
  }
  this->heartbeat_time = nullptr;
  this->ib             = nullptr;
  this->onetime        = nullptr;
  this->service        = nullptr;
  this->mysql_pool     = nullptr;
  this->pass.clear();

  printf("[client->%lld] %d disconnect\n", getTimeStamp(), this->id);
}

void Player::LoginSucces(std::function<void(bool&)> b_cb_f)
{
  auto uid = this->uid;
 
  mysql_pool->query("update `user` set `login_num` = `login_num` + 1 ,`pass` = CASE WHEN `login_num` < 5 THEN '"+this->pass+"' ELSE `pass` END , `login_time`=" + std::to_string(login_time) + ",`online_time`=" + std::to_string(login_time) +
                        " where `uid`=" + std::to_string(uid),
                    [=](call_back_data data) {});

  printf("[client->%lld] %d login->uid %lld\n", getTimeStamp(), this->id, this->uid);

  auto th           = this->ib;
  auto thisplayer   = this;
  auto thmysql_pool = mysql_pool;

  this->heartbeat_time = service->schedule(std::chrono::milliseconds(10000), [th, thisplayer, thmysql_pool, uid](io_service& service) {
    auto now_time = getTimeStamp();

  
    thmysql_pool->query("update `user` set `online_time`=" + std::to_string(now_time / 1000) + " where `uid`=" + std::to_string(uid),
                        [=](call_back_data data) {});

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

  mysql_pool->query("select `pass`,`login_num` from `user` where `uid`=" + std::to_string(uid), [=](call_back_data data) {
    bool b                  = false;

    if (data.size() == 0)
    {
     
      mysql_pool->query("INSERT INTO `user`(uid, reg_time, login_time, online_time, pass,login_num)VALUES(" + std::to_string(uid) + "," +
                            std::to_string(this->login_time) + "," + std::to_string(this->login_time) + "," + std::to_string(this->online_time) + ",'" +
                            pass.data() + "',0)",
                        [=](call_back_data data) { this->LoginSucces(b_cb_f); });
      return;
    }
    else if (data[0][0] != pass && atoi(data[0][1].data())>=5) 
    {
      b = false;
      b_cb_f(b);
      return;
    }
    this->LoginSucces(b_cb_f);
  });
}
