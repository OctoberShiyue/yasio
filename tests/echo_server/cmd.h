#ifndef _CMD_H_
#define _CMD_H_

std::map<std::string, std::function<void(std::vector<std::string> args)>> gCmds;

void HelpCmd(std::vector<std::string> args)
{
  std::cout << "Usage: [options]\n";
  std::cout << "Options:\n";
  std::cout << "  -relua Restarting the Lua Engine\n";
  std::cout << "  -lua [lua scripts] run lua sciprts\n";
}

void InitCmd()
{
  gCmds["-h"] = HelpCmd;
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