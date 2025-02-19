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
#include "minidump.h"
#include "cmd.h"
extern "C" {
#include "log.h"
}
using namespace yasio;

io_service* gservice = nullptr; // the weak pointer

LuaInit* gluainit = nullptr;

std::map<int, Player*> gPlayers;
std::map<INT64, bool> gPlayers_have_uid;
ConnectionPool* gmysql_pool = nullptr;
std::string gmysql_host;
std::string gmysql_user;
std::string gmysql_pass;
std::string gmysql_db;
int gmysql_port;

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
        printf("uid error=%lld\n", uid);
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

void run_echo_server(const char* protocol)
{
  std::ifstream inputFile("config.json");
  if (!inputFile.is_open())
  {
    std::cerr << "Could not open the config.json file!" << std::endl;
    return ;
  }
  nlohmann::json configData;
  try
  {
    inputFile >> configData;
  }
  catch (nlohmann::json::parse_error& e)
  {
    std::cerr << "Parse config.json error: " << e.what() << std::endl;
    return ;
  }
  inputFile.close();

  io_hostent endpoints[] = {{configData["service"]["ip"].get<std::string>().c_str(), configData["service"]["port"].get<u_short>()}};
  io_service server(endpoints, 1);
  gservice = &server;

  gmysql_pool = new ConnectionPool(gservice);
  gluainit    = new LuaInit(gmysql_pool, &gPlayers);
  gluainit->setService(gservice);
  gmysql_pool->setLuaInit(gluainit);
  gmysql_pool->host = configData["mysql"]["host"].get<std::string>().c_str();
  gmysql_pool->user = configData["mysql"]["user"].get<std::string>().c_str();
  gmysql_pool->pass = configData["mysql"]["pass"].get<std::string>().c_str();
  gmysql_pool->db   = configData["mysql"]["db"].get<std::string>().c_str();
  gmysql_pool->port = configData["mysql"]["port"].get<int>();
  gmysql_pool->init();
  gluainit->init();


  server.set_option(YOPT_S_NO_NEW_THREAD, 1);
  server.set_option(YOPT_C_MOD_FLAGS, 0, YCF_REUSEADDR, 0);

  signal(SIGINT, handle_signal);

  deadline_timer timer(server);
  timer.expires_from_now(std::chrono::seconds(1));
  timer.async_wait_once([=](io_service& server) {
    server.set_option(YOPT_C_UNPACK_PARAMS, 0, 65535, 0, 4, 0);
    printf("[server][%s] open server %s:%u ...\n", protocol, endpoints[0].get_ip().c_str(), endpoints[0].get_port());
    if (cxx20::ic::iequals(protocol, "udp"))
      server.open(0, YCK_UDP_SERVER);
    else if (cxx20::ic::iequals(protocol, "kcp"))
      server.open(0, YCK_KCP_SERVER);
    else
      server.open(0, YCK_TCP_SERVER);
  });

  gservice->schedule(std::chrono::milliseconds(60000), [=](io_service& service) {
    printf("[msg->%lld]player_num=%d\n", getTimeStamp() / 1000, gPlayerNum);
    log_trace("player_num=%d", gPlayerNum);
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
            catch (const std::exception&)
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
          auto id      = ev->source_id();
          auto p       = new Player(gservice, ev->source(), gmysql_pool, id);
          gPlayers[id] = p;
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
  std::thread([]() {
    CatchAndWriteMiniDump(
        [=]() {
          log_add_fp(fopen("log_trace.txt", "w+"), LOG_TRACE);
          log_add_fp(fopen("log_error.txt", "w+"), LOG_ERROR);
          log_trace("service run....");
          run_echo_server("tcp");
          printf("service error \n");
          return 0;
        },
        nullptr);
  }).detach();
  InitCmd();
  return 1;
}