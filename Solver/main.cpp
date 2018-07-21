#include "system.h"
#include "trace_writer.h"

using namespace std;

int main(int argc, char** argv)
{
	System::ParseArgs(argc, argv);
	auto out_file = System::GetArgValue("out");
	auto in_file = System::GetArgValue("in");

	Matrix *model = new Matrix();
	if (!model->load_from_file(in_file.c_str()))
	{
		fprintf(stderr, "Failed to load model '%s'", in_file.c_str());
		return 1;
	}

	TraceWriter tw(out_file.c_str(), model->R);

	string solver = "stupid"; // default solver
	if (System::HasArg("solver"))
		solver = System::GetArgValue("solver");
	auto solver_f = GetSolver(solver);
	if (!solver_f)
	{
		fprintf(stderr, "Unsupported solver: %s", solver.c_str());
		return 3;
	}

	solver_f(*model, tw);

	Assert(tw.get_filled_count() == model->get_filled_count());

	printf("%lld", tw.get_energy()); // print total energy spent
	return 0;
}
