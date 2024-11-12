#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "yasio/yasio.hpp"
#include <signal.h>

using namespace yasio;

static yasio::highp_time_t getTimeStamp() { return yasio::highp_clock<yasio::system_clock_t>() / 1000; }

class Player {
public:
  Player(io_service* service, io_base* th);
  ~Player();

  void Login(int64_t uid);

  void UpdateHeartbeat() { this->online_time = getTimeStamp(); };

  int64_t GetUid() const { return this->uid; };

  highp_time_t online_time;
  highp_time_t login_time;

  cxx17::string_view aes_key = "";

private:
  unsigned int id;
  int64_t uid;

  io_base* ib;
  io_service* service;

  deadline_timer_ptr onetime;

  deadline_timer_ptr heartbeat_time;
};
#endif