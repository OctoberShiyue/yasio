#include "yasio/yasio.hpp"
#include "player.h"
#include <signal.h>
#include <map>
#include <mysql/include/mysql.h>
#include <rsa/rsa.h>
#include <rsa/hex.h>
#include <windows.h>
#include <openssl/aes.h>
#include <iostream>

using namespace yasio;

io_service* gservice = nullptr; // the weak pointer

std::map<int, Player*> gPlayers;
std::map<int, bool> gPlayers_have_uid;

cxx17::string_view gpasswd = "shiyue is a powerful!";


// 辅助函数：填充字符串到16字节的倍数
std::string padString(const std::string& str)
{
  size_t paddingLength = AES_BLOCK_SIZE - (str.size() % AES_BLOCK_SIZE);
  return str + std::string(paddingLength, paddingLength);
}

// 示例函数：AES-256 ECB加密
std::string encryptAESECB(const std::string& plaintext, const unsigned char* key)
{
  std::string paddedText = padString(plaintext);
  std::string ciphertext(paddedText.size(), '\0');

  AES_KEY encryptKey;
  AES_set_encrypt_key(key, 256, &encryptKey);

  for (size_t i = 0; i < paddedText.size(); i += AES_BLOCK_SIZE)
  {
    AES_encrypt(reinterpret_cast<const unsigned char*>(&paddedText[i]), reinterpret_cast<unsigned char*>(&ciphertext[i]), &encryptKey);
  }

  return ciphertext;
}

// 示例函数：AES-256 ECB解密
std::string decryptAESECB(const std::string& ciphertext, const unsigned char* key)
{
  std::string decryptedText(ciphertext.size(), '\0');

  AES_KEY decryptKey;
  AES_set_decrypt_key(key, 256, &decryptKey);

  for (size_t i = 0; i < ciphertext.size(); i += AES_BLOCK_SIZE)
  {
    AES_decrypt(reinterpret_cast<const unsigned char*>(&ciphertext[i]), reinterpret_cast<unsigned char*>(&decryptedText[i]), &decryptKey);
  }

  // 去除填充
  size_t paddingLength = decryptedText.back();
  return decryptedText.substr(0, decryptedText.size() - paddingLength);
}



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
  uint16_t value1;
  int64_t value2;
  bool value3;
  float value4;
  double value6;
  int64_t uid;
  cxx17::string_view data;
  cxx17::string_view passwd = gpasswd;
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

    id     = ibs->read<int8_t>();
    value1 = ibs->read<uint16_t>();
    value2 = ibs->read_ix<int64_t>();
    value3 = ibs->read<bool>();
    value4 = ibs->read<float>();
    value6 = ibs->read<double>();
    uid    = ibs->read_ix<int64_t>();
    data   = ibs->read_v();
    passwd = ibs->read_v();
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
    obs->write<uint16_t>(value1);
    obs->write_ix<int64_t>(value2);
    obs->write<bool>(value3);
    obs->write<float>(value4);
    obs->write<double>(value6);
    obs->write_ix<int64_t>(uid);
    obs->write_v(data);
    obs->write_v(passwd);

    obs->pop<uint32_t>(len_pos, obs->length());
  }
};

enum
{
  SERVER_LOGIN         = 1, // 登录
  SERVER_HEARTBEAT     = 2, // 心跳
  SERVER_LOGIN_SUCCESS = 3, // 登录成功
};

bool RunFunc(io_event* ev, DataPacket* packet)
{
  if (!packet->passwd._Equal(gpasswd))
    return true; // 有可能是网络传输有问题导致验证失败，所以没必要断开链接

  auto tid = ev->transport()->id();

  auto p = gPlayers[tid];

  if (packet->id != SERVER_LOGIN && p->GetUid() <= 10000)
  {
    return false;
  }

  switch (packet->id)
  {

    case SERVER_LOGIN: {
      auto uid = packet->uid;
      if (uid <= 10000 || gPlayers_have_uid[uid])
      {
        return false;
      }
      p->Login(uid);
      gPlayers_have_uid[uid] = true;

      DataPacket pd{};

      pd.id     = SERVER_LOGIN_SUCCESS;
      pd.value1 = 996;
      pd.data   = "test data!";

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

  // !important, because we set YOPT_S_NO_NEW_THREAD, so need a timer to start server
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
  // server.schedule(std::chrono::milliseconds(100), [=](io_service& service) {
  //   RSA rsa;

  //   // 1024字节的rsa公私钥 可以加密 128字节

  //   // 设置公钥指数 e
  //   rsa.set_public_exp("10001");

  //   // 设置公钥模数 n
  //   rsa.set_modulus(
  //       "a87441ebe810751e23ec1341315b0f3a87cb58f8e96b1ccaf03f5a6b7258c4dad563f2f533e04759a7e954c9a7e8ecd8f161a2830f5dc4e9dc66253aff85ac744940d3685873"
  //       "07b64ba00c7b02d4df6583057126d1960591078be9a1c212bf54571f1e9a30525010ca5e93329010545966c569d6b58b38502d55d4096bf8e26d");

  //   // 设置私钥指数 d 这个私钥只能放在安全环境里使用
  //   rsa.set_private_exp(
  //       "53e8dd316a7e50287c524ae10d79c3632f633e6576b7f136b1678d5dba2eb7981df5547f89a0ad49de971eb1f85ed123db50fc0776af09b8481de56bb6fe5a011fdd8ef2"
  //       "becca9fc1d480cb3ab5249c67ac87b2bac81b668ee837dc099d6ab1a1fe32cd8cbff192fca05e18b2e4291379bb1c167ef905b43548655be732ceec1");

  //   // 源文
  //   std::string source = "hello worlds!测试中文行不行！ 字符串1   -";

  //   // 转16进制
  //   std::string source_hex = BinToHex(source);

  //   // 加密
  //   std::string password_hex = rsa.encode(source_hex);

  //   // 解密 再解码16进制
  //   std::string decode = HexToBin(rsa.decode(password_hex));

  //   printf("源文[%s]\n", source.c_str());

  //   printf("密文[%s]\n", password_hex.c_str());

  //   printf("解密文[%s]\n", decode.c_str());

  //   // 根据源文 生成签名
  //   std::string sign = rsa.get_sign(source);

  //   // 验证签名 是否跟 源文一致
  //   if (rsa.check_sign(source, sign))
  //   {
  //     printf("签名验证成功\n");
  //   }
  //   else
  //   {
  //     printf("签名验证失败\n");
  //   }

  //   std::string plaintext = "最常见的定义是主题是文章中通过具体材料所表达的基本思想。这个定义由来已久，似无庸置疑，但仔细想来，它似有片面之嫌。常识告诉我们文章是表情达意的工具。这个定义只及达意（表达的基本思想），而不及表情，岂不为缺漏？或谓达意即表情？若然，岂不是说情感与思想等同？但心理学研究早已证明，情感是与思维不同的心理过程，它具有独特的主观体验的形式和外部表现的形式，具有极为复杂的社会内容。虽然思想左右情感，但情感也会左右思想。详而言之，在实际心理过程中，有时思想是主流，有时情感是主流，尽管二者不可割裂。美国的J.M.索里C.W.特尔福德在《教育心理学》中指出当全部反应在性质上主要是情感反应时（主要是内脏的），观念性的期望与知觉的和概念的意义（主要是神经的）同样也可以成为全部反应的组成部分。反之亦然。心理过程中思想与情感所占地位的不同，外化或表现为不同文体中主题类型的不同。在逻辑类文章中，是理为主，在形象类文章中，是情为主。文论上说的辞以情发就是指的后者情形。各种形象类或文艺类文章，旨（主题）在表情言志，以情感人（不同于逻辑类的以理服人）。写这类文章，是情动于中而形于外，发乎情——能憎，能爱，始能文（鲁迅），终乎情——情尽言止。所以，为使主题或主旨的定义更有涵盖性，定义的中心词就应是思想与情感二者，即定义为主题是作者在文章中通过一系列精心选择剪裁并编织起来的具体材料所表达的最主要的思想和倾向（倾向就是对生活现实的憎爱情感或态度）。这样，定义对逻辑或形象类文章都适用。实际上，人们（例如语文教师）在分析归纳一些文章的主题时，通常总是说本文通过××（材料与表达方式），表达了作者×××的思想，抒发了作者×××的感情（有的虽不写抒发了××之情，但那个主题句却是把理志情融为一体，包含情志内容的）。可见只言思不言情的主题定义，也与主题这一术语在实际使用中的内涵不相吻合，而修改后的定义则可避免这一点。将主题定义的中心词改成思想与倾向，虽只是一词之增，但由于它符合作文心理过程的实际，符合文章内容的实际，因而无论对写作实践或阅读实践，都具有重要的意义。写作，尤其是文艺创作，正如黑格尔所说一方面求助于常醒的理解力，另一方面也要求助于深厚的心胸和灌注生气的情感。树立了主题是最主要的思想和倾向的深刻观念，将使习作者更自觉地用两条腿走路，更自觉地酝情发思，使二者相互渗透，相互激发。这就是情感思维。在情感思维中，情之所至，材料跃然，思如流水（联想和想象的纽带就是情感）。作家的经验证明正是在情思激越时，妙笔才能生花，写出文情并茂的传世之作。即使是写逻辑类的论说文，也当如朱光潜先生所说也还是要动一点情感，要用一点形象思维。如果把主题仅仅定义为主要思想，就会暗示人们去写所谓零度风格的文章。而零度风格的文章既不易写成，更不会打动读者（零度风格，zero style，参见朱光潜《漫谈说理文》）。阅读呢？固然，阅读要通过概念判断推理去评析，但首先要通过形象情境和美感等去鉴赏。主题仅仅是主要思想的观念，会暗示人们将阅读的注意力投向理性分析，而忽视形象思维（不少学生形象思维能力差，与他们自小就接受的主题就是主要思想这个定义不无关系）。其实，阅读应当交错地运用抽象思维与形象思维，领会文中情理相生的旨趣。鉴赏文学作品，既要借助想象（与深厚的心胸和灌注生气的情感相关联），又要借助分析综合和概括（与常醒的理解力相关联），挖掘作品的思想意义和所蕴含的哲理。这才能发挥文学的认识作用教育作用和美感作用的整体功能。定义的中心词何以用倾向而不用情感？这是因为倾向除含有憎爱之情外，还有态度趣向等几个义项，即有更广的外延。文体不同，内容不同，情感的类型也各异。各种思>；情的文体（各种应用文政论文学术论文等），狭义的情感（憎爱之情）色彩并不浓，蕴含于文中的，主要是某种志向愿望态度或精神。而这些广义的情感，均可用倾向这一术语来指称。例如，一些学术论文，与其说内中蕴含一种情感，勿宁说蕴含着一种倾向，一种执着地探索并证明真理的欲望志向和求实精神。因为通常总是把情感理解为狭义的，所以用倾向可以使定义对各类文体都适用。材料常见的一个定义是材料是提供文章内容和表达主题的事物和观念。严格地说，事物并不是材料尚未反映到头脑中的事物不会是材料；已经反映到头脑中（或写入文章中）的事物，已是一种观念，一种关于事物的感性或理性的认识。这是唯物论的常识。与此相一致，人们有文章是客观事物的反映的正确命题反映二字，不独指文章的观点，也指其中的材料。也正因为如此，人们评价文中材料时才有真与假片面与全面等标准。如果材料的外延包含与观念相对的事物本身，那材料（事物）就没有真假偏全等区别了。所以，材料是事物的说法不能成立。另一种有影响的定义是材料是从生活中搜集摄取以及写入文章中的一系列事实或论据。这个定义含有另一种毛病。事实或论据，显系分指两类文体中的材料文艺类的材料——事实，论说类的材料——论据。但是，文艺类的材料不尽是事实（事实一般总是指对事物现象过程的直接反映，属于感性认识），也有与事实相对的理念（指思想观点，即理性认识）。例如，文艺作品中经常穿插一些引用先哲的话语或作者关于生活哲理的直接议论等，用以支持作品的主题。这一类材料，就属于与事实相对的理念。所以，用于分指的这个定义中的事实，实在缺乏概括性。若定义中的事实或论据不是分指，而是概指，同样不能成立文艺文既然非论，其中虽有议论，也不宜称为论据；至于论说文，其中的材料固可称为论据，但论据与事实不是同一平面上的概念，而是属种关系的概念——论据划分为事实论据和理论论据两类。处于上下位关系的两个术语（事实和论据），用或或和连接都是不妥的。对材料怎样下定义才臻于严格和科学？上举定义中的事物和观念，本意在指出两种类型的材料感性材料——事物，理性材料——观念。倘从这个角度下定义，并与认识论相一致，似可说材料是作者形成或表达特定主题所依赖和采用的一系列感性和理性认识。或者材料是作者在形成或表达特定主题时摄取使用的各种信息——感知和理念。还应提及的是，上边这个定义中形成或表达特定主题这个修饰性成分绝不可省，因为它揭示了材料的自身规定性——与主题的相对性或相互依存关系。材料，总是特定主题的材料。换言之，只是在与特定主题的对立联系中，特定的信息（感知和理念）才称之为材料。因此，在此篇为材料者（如某种观念），在彼篇可以是主题；在此篇为主题者，也可以是另一篇另一主题的材料。这就是材料和主题的相互依存性与相对性。我们在给主题下定义时揭示了二者的这种辩证关系，指出主题是一系列具体材料表达出来的最基本的思想和倾向。这是因为没有特定具体材料的表现就没有主题。同样，在给材料下定义时，也绝不可忽略这一点。形成或表达特定主题——这是材料自身存在的根据，是材料的本质属性之一。结构常见的一种提法是简洁地说，结构就是文章的内部组织构造。这个定义看似天经地义，其实违背逻辑。因为逻辑上的定义公式是种差+邻近的属，而这个定义的中心词组织构造并不是结构邻近的属，而只是结构的同义词。我们不能知道构造比结构多了点什么，所以这个定义实等于说结构，就是文章的内部结构。可惜这个定义一直为一些论著所沿用。有的书在结构章没说结构是什么，只指出结构是文章的骨架，似以此作为结构的定义。骨架，确实很形象地表达了结构的特征和作用，但这只是一个比喻，比喻永远不能成为定义，因shiyue";
  //   unsigned char key[] = "0123456789abcdef0123456789abcdef";


  //   std::string ciphertext    = encryptAESECB(plaintext, key);
  //   std::string decryptedtext = decryptAESECB(ciphertext, key);

  //   // std::cout << "Original: " << plaintext << std::endl;
  //   /*std::cout << "Encrypted: ";
  //   for (unsigned char c : ciphertext)
  //   {
  //     printf("%02x", c);
  //   }*/
  //   // std::cout << std::endl;
  //   std::cout << "Decrypted: " << decryptedtext << std::endl;
  //   return false;
  // });
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
          auto p                          = new Player(gservice, ev->source());
          gPlayers[ev->transport()->id()] = p;
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
        break;
      }
    }
  });
}

int main(int argc, char** argv)
{
  // 设置控制台输出为 UTF-8 编码
  SetConsoleOutputCP(CP_UTF8);
  MYSQL* mysql = mysql_init(nullptr);

  if (nullptr == mysql_real_connect(mysql, "124.223.83.199", "testorpg", "mh4wabJCGnLMWH7E", "testorpg", 3306, nullptr, 0))
  {
    printf("[mysql] connect fail\n");
    return 0;
  }

  printf("[mysql] connect success\n");

  run_echo_server("0.0.0.0", 18199, "tcp");
  return 0;
}