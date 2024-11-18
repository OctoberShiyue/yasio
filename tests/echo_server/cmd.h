#ifndef _CMD_H_
#define _CMD_H_
#include <connectionPool.h>

std::map<std::string, std::function<void(std::vector<std::string> args)>> gCmds;

void HelpCmd(std::vector<std::string> args)
{
  std::cout << "Usage: [options]\n";
  std::cout << "Options:\n";
  std::cout << "  -lua [scripts code] run lua sciprts\n";
}

void LoadLuaCodeCmd(std::vector<std::string> args)
{
  std::ostringstream oss;
  for (const auto& str : args)
  {
    oss << str + " "; 
  }
  std::string code = oss.str();

  ConnectionPool::loadLuaCode(code);
}

void InitCmd()
{
  gCmds["-h"]   = HelpCmd;
  gCmds["-lua"] = LoadLuaCodeCmd;
  std::string input;
  while (true)
  {
    std::getline(std::cin, input);

    std::istringstream iss(input);
    std::string command;
    std::vector<std::string> args;
    iss >> command;

    std::string arg;
    while (iss >> arg)
    {
      args.push_back(arg);
    }
    if (gCmds[command])
    {
      gCmds[command](args);
    }
  }
}

#endif