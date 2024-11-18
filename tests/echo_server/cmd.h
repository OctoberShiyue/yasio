#ifndef _CMD_H_
#define _CMD_H_
#include <connectionPool.h>
#include <conio.h>
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

std::string getInputFromDialog()
{
  std::ofstream script("input.vbs");
  script << "Dim input\n";
  script << "input = InputBox(\"Please enter your input:\" & vbCrLf & \"-h: help\" & vbCrLf & \"-lua [scripts code] run lua scripts\", \"Input Box\")\n";
  script << "If input <> \"\" Then\n";
  script << "   WScript.Echo input\n";
  script << "End If\n";
  script.close();

  std::string result;
  char buffer[128];
  FILE* pipe = _popen("cscript //nologo input.vbs", "r");

  if (pipe)
  {
    while (fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
      result += buffer;
    }
    _pclose(pipe);
  }

  std::remove("input.vbs");

  return result;
}

void InitCmd()
{
  gCmds["-h"]   = HelpCmd;
  gCmds["-lua"] = LoadLuaCodeCmd;

  while (true)
  {
    if (_kbhit())
    {
      int ch = _getch();
      if (ch == 0 || ch == 224)
      {
        ch = _getch();
        if (ch == 63)
        {
          std::string input = getInputFromDialog();
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
    }
    Sleep(100);
  }
}

#endif