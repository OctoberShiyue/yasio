#include "yasio/yasio.hpp"
#include "player.h"
#include <signal.h>
#include <map>
#include <windows.h>
#include <iostream>
#include <base64/base64.h>
#include <json/json.hpp>
#include <connectionPool.h>

using namespace yasio;

io_service* gservice        = nullptr; // the weak pointer
ConnectionPool* gmysql_pool = nullptr;

std::map<int, Player*> gPlayers;
std::map<int, bool> gPlayers_have_uid;

int gPlayerNum = 0;

enum
{
  SERVER_LOGIN         = 1, // 登录
  SERVER_HEARTBEAT     = 2, // 心跳
  SERVER_LOGIN_SUCCESS = 3, // 登录成功
};

struct DataPacket {

  struct Header {
    uint16_t command_id;
    uint16_t version;
    int32_t reserved;
    int32_t reserved2_low;
    int32_t reserved2_high;
  };
  Header header;

  int8_t id; // 信息

  nlohmann::json data;

  DataPacket(){

  };
  DataPacket(yasio::ibstream* ibs)
  {
    ibs->seek(4, SEEK_CUR);
    header.command_id     = ibs->read<uint16_t>();
    header.version        = ibs->read<uint16_t>();
    header.reserved       = ibs->read<int32_t>();
    header.reserved2_low  = ibs->read<int32_t>();
    header.reserved2_high = ibs->read<int32_t>();

    id   = ibs->read<int8_t>();
    data = nlohmann::json::parse(Base64::decode(ibs->read_v().data()));
  }

  void setObstream(yasio::obstream* obs)
  {
    auto len_pos = obs->push<uint32_t>();
    obs->write<uint16_t>(101); // CID_SIMPLE1
    obs->write<uint16_t>(1);
    obs->write<int32_t>(0);
    obs->write<int32_t>(0);
    obs->write<int32_t>(0);

    obs->write<int8_t>(id);
    obs->write_v(Base64::encode(data.dump()));

    obs->pop<uint32_t>(len_pos, obs->length());
  }
};

bool RunFunc(io_event* ev, DataPacket* packet)
{

  auto tid = ev->transport()->id();

  auto p = gPlayers[tid];

  if (packet->id != SERVER_LOGIN && p->GetUid() <= 10000)
  {
    return false;
  }

  switch (packet->id)
  {

    case SERVER_LOGIN: {
      auto uid = (int64_t)packet->data["uid"];
      if (uid <= 10000 || gPlayers_have_uid[uid])
      {
        return false;
      }
      //// 获取开始时间点
      // auto start = std::chrono::high_resolution_clock::now();
      if (!p->Login(uid, (std::string)packet->data["pw"]))
      {
        return false;
      }
      // 获取结束时间点
      auto end = std::chrono::high_resolution_clock::now();

      //// 计算耗时
      // std::chrono::duration<double> elapsed = end - start;
      // std::cout << "Elapsed time: " << elapsed.count() << " seconds" << std::endl;
      gPlayers_have_uid[uid] = true;

      DataPacket pd{};

      pd.id = SERVER_LOGIN_SUCCESS;
      nlohmann::json njson;
      njson["value1"] = 9993;
      njson["pw"]     = p->pass.data();
      pd.data         = njson;

      auto obs = cxx14::make_unique<yasio::obstream>();
      pd.setObstream(obs.get());
      gservice->write(ev->transport(), obs->data(), obs->length());

      break;
    }
    case SERVER_HEARTBEAT: {
      p->UpdateHeartbeat();
      break;
    }
  }
  return true;
}

void handle_signal(int sig)
{
  if (gservice && sig == 2)
  {
    gservice->close(0);
  }
}

// 6: TCP server, 10: UDP server, 26: KCP server
void run_echo_server(const char* ip, u_short port, const char* protocol)
{
  io_hostent endpoints[] = {{ip, port}};
  io_service server(endpoints, 1);
  gservice = &server;
  server.set_option(YOPT_S_NO_NEW_THREAD, 1);
  server.set_option(YOPT_C_MOD_FLAGS, 0, YCF_REUSEADDR, 0);

  signal(SIGINT, handle_signal);

  deadline_timer timer(server);
  timer.expires_from_now(std::chrono::seconds(1));
  timer.async_wait_once([=](io_service& server) {
    server.set_option(YOPT_C_UNPACK_PARAMS, 0, 65535, 0, 4, 0);
    printf("[server][%s] open server %s:%u ...\n", protocol, ip, port);
    if (cxx20::ic::iequals(protocol, "udp"))
      server.open(0, YCK_UDP_SERVER);
    else if (cxx20::ic::iequals(protocol, "kcp"))
      server.open(0, YCK_KCP_SERVER);
    else
      server.open(0, YCK_TCP_SERVER);
  });

  gservice->schedule(std::chrono::milliseconds(60000), [=](io_service& service) {
    printf("[msg->%lld]player_num=%d\n", getTimeStamp()/1000, gPlayerNum);
    return false;
  });

  server.start([&](event_ptr ev) {
    switch (ev->kind())
    {
      case YEK_PACKET: {
        if (ev->status() == 0)
        {
          auto& pkt = ev->packet();
          if (!pkt.empty())
          {
            auto ibs = cxx14::make_unique<yasio::ibstream>(forward_packet((packet_t&&)pkt));

            try
            {
              DataPacket packet(ibs.get());
              if (!RunFunc(ev.get(), &packet))
              {
                gservice->close(ev->transport());
              }
            }
            catch (const std::exception&)
            {
              server.close(ev->transport());
            }
          }
        }
        break;
      }
      case YEK_CONNECT_RESPONSE: {
        if (ev->status() == 0)
        {
          auto p                          = new Player(gservice, ev->source(), gmysql_pool);
          gPlayers[ev->transport()->id()] = p;
          gPlayerNum++;
        }
        break;
      }
      case YEK_CONNECTION_LOST: {
        auto id      = ev->transport()->id();
        auto p       = gPlayers[ev->transport()->id()];
        auto uid     = p->GetUid();
        gPlayers[id] = nullptr;
        if (gPlayers_have_uid[uid])
          gPlayers_have_uid[uid] = false;

        delete p;
        gPlayerNum--;
        break;
      }
    }
  });
}

int main(int argc, char** argv)
{
  // 设置控制台输出为 UTF-8 编码
  SetConsoleOutputCP(CP_UTF8);

  gmysql_pool = new ConnectionPool("127.0.0.1", "root", "root", "testorpg", 3306,
#ifdef NDEBUG
                                   10
#else
                                   1
#endif
  );

  run_echo_server("0.0.0.0", 18199, "tcp");
  return 0;
}