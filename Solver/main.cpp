#include "system.h"
#include "trace_writer.h"

int main(int argc, char** argv)
{
	System::ParseArgs(argc, argv);
	auto fname = System::GetArgValue("out");
	TraceWriter tw(fname.c_str(), 10);
	tw.halt();
	return 0;
}