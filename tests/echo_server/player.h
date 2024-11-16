#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "yasio/yasio.hpp"
#include <signal.h>
#include <connectionPool.h>

using namespace yasio;

static yasio::highp_time_t getTimeStamp() { return yasio::highp_clock<yasio::system_clock_t>() / 1000; }

class Player {
public:
  Player(io_service* service, io_base* th, ConnectionPool* mysql_pool, unsigned int id);
  ~Player();

  void LoginSucces(std::function<void(bool&)> b_cb_f);

  void Login(int64_t uid, std::string pass, std::function<void(bool&)> b_cb_f);

  void UpdateHeartbeat() { this->online_time = getTimeStamp(); };

  transport_handle_t GetTransport() { return (transport_handle_t)this->ib; };

  int64_t GetUid() const { return this->uid; };
  unsigned int GetId() const { return this->id; };

  highp_time_t online_time;
  highp_time_t login_time;

  std::string pass = "";

  ConnectionPool* mysql_pool;

private:
  unsigned int id = 0;
  int64_t uid     = 0;

  io_base* ib         = nullptr;
  io_service* service = nullptr;

  deadline_timer_ptr onetime = 0;

  deadline_timer_ptr heartbeat_time = 0;
};
#endif