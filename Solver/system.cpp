//-----------------------------------------------------------------------------
// File: system.cpp
//
// Desc:
//
// Copyright (c) 2009 Damir Akhmetzyanov
//-----------------------------------------------------------------------------

#include "system.h"
#include <sys/stat.h>
#include <time.h>
#ifdef _MSC_VER
#include <windows.h>
#endif

#pragma warning( disable : 4996 ) // disable deprecated warning

	std::map<std::string, std::vector<std::string> > System::args;

	void System::ParseArgs(const std::vector<std::string> &v)
	{
		args.clear();
		std::string last;
		for (std::vector<std::string>::const_iterator it = v.begin(); it != v.end(); it++)
		{
			std::string s = *it;
			if (s.size() > 1 && s[0] == '-')
			{
				last = s.substr(1);
				if (args.find(last) == args.end())
					args[last] = std::vector<std::string>();
			}
			else
			{
				if (last != "")
				{
					args[last].push_back(s);
				}
			}
		}
	}

    void System::ParseArgs(int argc, char *argv[])
    {
		std::vector<std::string> v;
#ifdef _MSC_VER
		{
			int argc;
			LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
			for (int i = 1; i < argc; i++)
			{
				char buf[2048];
				WideCharToMultiByte(ANSI_CODEPAGE, 0, argv[i], -1, buf, 2048, NULL, NULL);
				v.push_back(buf);
			}
			LocalFree(argv);
		}
#else
		for (int i = 0; i < argc; i++)
			v.push_back(argv[i]);
#endif
		ParseArgs(v);
    }


    bool System::HasArg(const char *arg_name)
    {
        return args.find(arg_name) != args.end();
    }

    std::string System::GetArgValue(const char *arg_name)
    {
        if (!HasArg(arg_name)) return "";
        if (args[arg_name].size() == 0) return "";
        return args[arg_name][0];
    }

    std::vector<std::string> System::GetArgValues(const char *arg_name)
    {
    	if (!HasArg(arg_name)) return std::vector<std::string>();
    	return args[arg_name];
    }
