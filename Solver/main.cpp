
#include "system.h"
#include "trace_writer.h"
#include "reversu.h"


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
	//if (ptype != 'A') return 42;

	string in_file2;
	if (ptype == 'R')
	{
		auto idx = in_file.find("src");
		if (idx == string::npos) return 66;
		in_file2 = in_file;
		in_file2.replace(idx, 3, "tgt");
	}
	Matrix *model = new Matrix();
	Matrix *model2 = nullptr;

	if (!model->load_from_file(in_file.c_str()))
	{
		fprintf(stderr, "Failed to load model '%s'", in_file.c_str());
		return 1;
	}
	model->init_sums();

	if (!in_file2.empty())
	{
		model2 = new Matrix();
		if (!model2->load_from_file(in_file2.c_str()))
		{
			fprintf(stderr, "Failed to load model2 '%s'", in_file2.c_str());
			return 1;
		}
		Assert(model->R == model2->R);
		model2->init_sums();
	}

	string solver = "bfs"; // default solver
	if (System::HasArg("solver"))
		solver = System::GetArgValue("solver");
	auto solver_f = GetSolver(solver);
	if (!solver_f)
	{
		fprintf(stderr, "Unsupported solver: %s", solver.c_str());
		return 3;
	}

	if (ptype == 'A')
    {
		FileTraceWriter *tw = new FileTraceWriter(out_file.c_str(), model->R, nullptr);
		solver_f(nullptr, model, tw);

		tw->halt();
		Assert(tw->get_matrix().check_equal(*model));
		Assert(tw->get_filled_count() == model->get_filled_count());

		printf("%lld", tw->get_energy()); // print total energy spent
		delete tw;
		return 0;
    }

	else if (ptype == 'D')
    {
		const string rev_out_file = out_file + ".rev";
		FileTraceWriter *tw = new FileTraceWriter(rev_out_file.c_str(), model->R, nullptr);
		solver_f(nullptr, model, tw);
		tw->halt();
		Assert(tw->get_matrix().check_equal(*model));
		Assert(tw->get_filled_count() == model->get_filled_count());
		delete tw;

		tw = new FileTraceWriter(out_file.c_str(), model->R, model);
        reverse_trace(rev_out_file, tw);
        tw->halt();
		Assert(tw->get_filled_count() == 0);

		printf("%lld", tw->get_energy()); // print total energy spent
		delete tw;
		return 0;
	}

	// todo
    else if (ptype == 'R')
	{
		FileTraceWriter *tw = new FileTraceWriter(out_file.c_str(), model->R, model);

		if (model->check_equal(*model2))
		{
			// no operations needed
		}
		else if (System::HasArg("solver2"))
		{
			auto solver2 = System::GetArgValue("solver2");
			auto solver2_f = GetSolver(solver2);
			if (!solver2_f)
			{
				fprintf(stderr, "Unsupported solver: %s", solver2.c_str());
				return 4;
			}
			solver_f(model, nullptr, tw);
			solver2_f(nullptr, model2, tw);
		}
		else
		{
			solver_f(model, model2, tw);
		}

		tw->halt();
		Assert(tw->get_matrix().check_equal(*model));
		Assert(tw->get_filled_count() == model->get_filled_count());

		printf("%lld", tw->get_energy()); // print total energy spent
		delete tw;
		return 0;
	}
	else
		return 44;
}
