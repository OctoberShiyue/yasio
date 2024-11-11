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

  int64_t GetUid(){return this->uid;};

  highp_time_t online_time;
  highp_time_t login_time;

private:
  unsigned int id;
  int64_t uid;

  io_base* ib;
  io_service* service;

  deadline_timer_ptr onetime; // ��һ��������֤��ʱ��

  deadline_timer_ptr heartbeat_time; // ������ʱ��
};

Player::Player(io_service* service, io_base* th)
{
  this->ib      = th;
  this->id      = th->id();
  this->service = service;
  printf("%d �ͻ�������\n", this->id);

  // ��һ�ν������ӣ��ͻ��˺ͷ���˽������ӣ���Ҫ�����ÿͻ��˷��͵�¼��֤��������ǣ��������3�룬���Զ��Ͽ�����

  this->onetime = service->schedule(std::chrono::milliseconds(3000), [th](io_service& service) {
    service.close((transport_handle_t)th);
    return true;
  });
}

Player::~Player()
{
  if (this->ib == nullptr)
    return;
  if (this->heartbeat_time!=nullptr)
  {
    this->heartbeat_time->cancel();
  }
  this->heartbeat_time = nullptr;
  this->ib             = nullptr;
  this->onetime        = nullptr;

  printf("%d �ͻ��˶Ͽ�\n", this->id);
}

void Player::Login(int64_t uid)
{
  this->uid = uid;
  this->onetime->cancel();
  this->onetime     = nullptr;
  this->login_time  = getTimeStamp();
  this->online_time = getTimeStamp();

  printf("%d �ͻ��˵�¼UID��%lld\n", this->id, this->uid);

  auto th = this->ib;
  auto thisplayer = this;
  // ������ʱ����10��һ��
  this->heartbeat_time = service->schedule(std::chrono::milliseconds(10000), [th, thisplayer](io_service& service) {
    auto now_time = getTimeStamp();

    //printf("%d �ͻ��˵�¼UID��%lld=%lld\n", thisplayer->id, thisplayer->uid, now_time - thisplayer->online_time);

    // �������30�룬���Զ��Ͽ�����
    if (now_time - thisplayer->online_time <= 30000)
    {
      return false;
    }
    service.close((transport_handle_t)th);
    return true;
  });
}
