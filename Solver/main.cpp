
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

	Matrix *model = new Matrix();
	if (!model->load_from_file(in_file.c_str())) {
		fprintf(stderr, "Failed to load model '%s'", in_file.c_str());
		return 1;
	}
	model->init_sums();

	if (ptype == 'A' || ptype == 'U') {
        string solver = "bfs";
        if (System::HasArg("solver"))
            solver = System::GetArgValue("solver");
        auto solver_f = GetSolver(solver);
        if (!solver_f) {
            fprintf(stderr, "Unsupported solver: %s", solver.c_str());
            return 3;
        }

		FileTraceWriter *tw = new FileTraceWriter(out_file.c_str(), model->R, nullptr);
		solver_f(nullptr, model, tw);

		tw->halt();
		if (!tw->get_matrix().check_equal(*model))
		{
#ifdef DEBUG
			tw->get_matrix().dump("dump.mdl", { });
#endif
			delete tw;
			Assert(false);
		}
		//Assert(tw->get_filled_count() == model->get_filled_count());

		printf("%lld", tw->get_energy()); // print total energy spent
		delete tw;
		return 0;
	}

	else if (ptype == 'D') {
        // Construction
        string rev_out_file;
        if (System::HasArg("trace")) {
            rev_out_file = System::GetArgValue("trace");
        } else {
            string solver = "bfs";
            if (System::HasArg("solver"))
                solver = System::GetArgValue("solver");
            auto solver_f = GetSolver(solver);
            if (!solver_f) {
                fprintf(stderr, "Unsupported solver: %s", solver.c_str());
                return 3;
            }
            rev_out_file = out_file + ".rev";
            FileTraceWriter *tw = new FileTraceWriter(rev_out_file.c_str(), model->R, nullptr);
            solver_f(nullptr, model, tw);
            tw->halt();
            Assert(tw->get_matrix().check_equal(*model));
            Assert(tw->get_filled_count() == model->get_filled_count());
            delete tw;
        }

		// Reversing construction to deconstruction
		FileTraceWriter *tw = new FileTraceWriter(out_file.c_str(), model->R, model);
		reverse_trace(rev_out_file, tw);
		tw->halt();
		Assert(tw->get_filled_count() == 0);

		printf("%lld", tw->get_energy());
		delete tw;
		return 0;
	}

	else if (ptype == 'R') {
        auto idx = in_file.find("src");
		if (idx == string::npos) return 66;
		string tgt_file = in_file;
		tgt_file.replace(idx, 3, "tgt");

		Matrix *tgt_model = new Matrix();
		if (!tgt_model->load_from_file(tgt_file.c_str())) {
			fprintf(stderr, "Failed to load tgt_model '%s'", tgt_file.c_str());
			return 1;
		}
		Assert(model->R == tgt_model->R);
		tgt_model->init_sums();

		// Equivalent models -> fast path
		if (model->check_equal(*tgt_model)) {
			FileTraceWriter *tw = new FileTraceWriter(tgt_file.c_str(), tgt_model->R, tgt_model);
			tw->halt();
			printf("%lld", tw->get_energy());
			delete tw;
			return 0;
		}

		// Construction
        string solver = "bfs";
        string rev_out_file;
        if (System::HasArg("trace")) {
            rev_out_file = System::GetArgValue("trace");
        } else {
            if (System::HasArg("solver"))
                solver = System::GetArgValue("solver");
            auto solver_f = GetSolver(solver);
            if (!solver_f) {
                fprintf(stderr, "Unsupported solver: %s", solver.c_str());
                return 3;
            }
            rev_out_file = out_file + ".rev";
            FileTraceWriter *tw = new FileTraceWriter(rev_out_file.c_str(), model->R, nullptr);
            solver_f(nullptr, model, tw);
            tw->halt();
            Assert(tw->get_matrix().check_equal(*model));
            Assert(tw->get_filled_count() == model->get_filled_count());
            delete tw;
        }

		// Reversing construction to deconstruction
		FileTraceWriter *tw = new FileTraceWriter(out_file.c_str(), model->R, model);
		reverse_trace(rev_out_file, tw);
		Assert(tw->get_filled_count() == 0);

		// Construction
		string tgt_solver = solver;
		if (System::HasArg("solver2"))
			tgt_solver = System::GetArgValue("solver2");
		auto tgt_solver_f = GetSolver(tgt_solver);
		if (!tgt_solver_f) {
			fprintf(stderr, "Unsupported solver: %s", tgt_solver.c_str());
			return 3;
		}
		tgt_solver_f(nullptr, tgt_model, tw);
		tw->halt();
		Assert(tw->get_matrix().check_equal(*tgt_model));
		Assert(tw->get_filled_count() == tgt_model->get_filled_count());

		printf("%lld", tw->get_energy());
		delete tw;
		return 0;
	}

	else
		return 44;
}
