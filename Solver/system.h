#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "common.h"

class System
{
public:
	static void ParseArgs(const std::vector<std::string> &args);
	static void ParseArgs(int argc, char* argv[]);
	static bool HasArg(const char *arg_name);
	static std::string GetArgValue(const char *arg_name);
	static std::vector<std::string> GetArgValues(const char *arg_name);
private:
	static std::map<std::string, std::vector<std::string> > args;
};


#endif
