#include <player.h>

Player::Player(io_service* service, io_base* th)
{
  this->ib      = th;
  this->id      = th->id();
  this->service = service;
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

void Player::Login(int64_t uid)
{
  this->uid = uid;
  this->onetime->cancel();
  this->onetime     = nullptr;
  this->login_time  = getTimeStamp();
  this->online_time = getTimeStamp();

  printf("[client->%lld] %d login->uid %lld\n", getTimeStamp(), this->id, this->uid);

  auto th         = this->ib;
  auto thisplayer = this;

  this->heartbeat_time = service->schedule(std::chrono::milliseconds(10000), [th, thisplayer](io_service& service) {
    auto now_time = getTimeStamp();

    if (now_time - thisplayer->online_time <= 30000)
    {
      return false;
    }
    service.close((transport_handle_t)th);
    return true;
  });
}
