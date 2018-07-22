#include "system.h"
#include "trace_writer.h"

using namespace std;

char get_type(string s)
{
	for (u32 i = s.size(); i >= 0; i--) if (s[i] == '/' || s[i] == '\\')
		return s[i + 2];
	return s[1];
}

int main(int argc, char** argv)
{
	System::ParseArgs(argc, argv);
	auto out_file = System::GetArgValue("out");
	auto in_file = System::GetArgValue("in");

	char ptype = get_type(in_file);
	if (ptype != 'A') return 42;

	Matrix *model = new Matrix();

	if (!model->load_from_file(in_file.c_str()))
	{
		fprintf(stderr, "Failed to load model '%s'", in_file.c_str());
		return 1;
	}

	FileTraceWriter *tw = new FileTraceWriter(out_file.c_str(), model->R);

	string solver = "stupid"; // default solver
	if (System::HasArg("solver"))
		solver = System::GetArgValue("solver");
	auto solver_f = GetSolver(solver);
	if (!solver_f)
	{
		fprintf(stderr, "Unsupported solver: %s", solver.c_str());
		return 3;
	}

	solver_f(model, tw);

	if (ptype == 'D')
	{
		Assert(tw->get_filled_count() == 0);
	}
	else
	{
		Assert(tw->get_matrix().check_equal(*model));
	}
	Assert(tw->get_filled_count() == model->get_filled_count());

	printf("%lld", tw->get_energy()); // print total energy spent
	delete tw;
	return 0;
}
