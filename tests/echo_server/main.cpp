#include "yasio/yasio.hpp"
#include "player.h"
#include <signal.h>
#include <map>
#include <windows.h>
#include <iostream>
#include <base64/base64.h>
#include <json/json.hpp>
#include <connectionPool.h>
#include <luainit.h>
#include <datapacket.h>

using namespace yasio;

io_service* gservice        = nullptr; // the weak pointer
ConnectionPool* gmysql_pool = nullptr;
LuaInit* gluainit           = nullptr;

std::map<int, Player*> gPlayers;
std::map<int, bool> gPlayers_have_uid;

int gPlayerNum = 0;

bool RunFunc(io_event* ev, DataPacket* packet)
{

  auto tid = ev->transport()->id();

  auto p = gPlayers[tid];

  if (packet->id != SERVER_LOGIN && p->GetUid() <= 10000)
  {
    printf("login error\n");
    return false;
  }

  switch (packet->id)
  {

    case SERVER_LOGIN: {
      auto uid = (int64_t)packet->data["uid"];
      if (uid <= 10000 || gPlayers_have_uid[uid])
      {
        printf("uid error\n");
        return false;
      }
      gPlayers_have_uid[uid] = true;

      p->Login(uid, (std::string)packet->data["pw"], [=](bool b) {
        if (!b)
        {
          printf("pass error\n");
          gservice->close(p->GetTransport());
          return;
        }
        gluainit->notifyConnent(1, p, nullptr);
      });

      break;
    }
    case SERVER_HEARTBEAT: {
      p->UpdateHeartbeat();
      break;
    }
    default: {
      gluainit->notifyConnent(packet->id, p, packet);
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
  gservice    = &server;
  gmysql_pool = new ConnectionPool("127.0.0.1", "root", "root", "testorpg", 3306, gservice);
  gluainit    = new LuaInit(gservice, gmysql_pool, &gPlayers);

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
    printf("[msg->%lld]player_num=%d\n", getTimeStamp() / 1000, gPlayerNum);
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
            auto ibs = cxx14::make_unique<yasio::ibstream>(forward_packet((packet_t &&) pkt));

            try
            {
              DataPacket packet(ibs.get());
              if (!RunFunc(ev.get(), &packet))
              {
                gservice->close(ev->transport());
                printf("run error\n");
              }
            }
            catch (const std::exception& e)
            {
              server.close(ev->transport());
              printf(" error\n");
            }
            ibs = nullptr;
          }
        }
        break;
      }
      case YEK_CONNECT_RESPONSE: {
        if (ev->status() == 0)
        {
          auto p                    = new Player(gservice, ev->source(), gmysql_pool, ev->source_id());
          gPlayers[ev->source_id()] = p;
          gPlayerNum++;
        }
        break;
      }
      case YEK_CONNECTION_LOST: {
        auto id = ev->source_id();
        auto p  = gPlayers[id];
        gluainit->notifyConnent(0, p, nullptr);
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

  run_echo_server("0.0.0.0", 18199, "tcp");

  // main一个线程
  // 服务端,lua一个线程【完成】
  // lua需要重启，和执行lua代码功能，需要封装，计时器【完成】|发送数据【完成】|接受数据|执行sql
  // 数据库查询一个线程【完成】

  return 0;
}