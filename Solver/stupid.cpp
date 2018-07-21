#include "trace_writer.h"

int stupid_solver(const Matrix &target, TraceWriter &w)
{
	w.halt();
	return 0;
}

REG_SOLVER("stupid", stupid_solver);