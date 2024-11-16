
#ifndef _DATAPACKET_H_
#define _DATAPACKET_H_
#include <cstdint>
#include "yasio/yasio.hpp"
#include <json/json.hpp>
#include <base64/base64.h>
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
    data = nlohmann::json::parse(Base64::decode(ibs->read_v().data()).c_str());
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
#endif