#include "trace_writer.h"

using namespace std;

struct StupidSolver
{

	void dfs(Point p)
	{
		reach_cell(b, p, &cur, w);
		w->fill(b->pos, p);
		cur[p] = true;
		for (auto d : kDeltas6)
		{
			Point t = p + d;
			if (!cur.is_valid(t)) continue;
			if ((*m)[t] && !cur[t]) dfs(t);
		}
	}

	int solve()
	{
		// initial position
		int x0 = -1, z0 = 1;
		int dist = 1000;
		for (int x = 0; x < R; x++)
			for (int z = 0; z < R; z++)
				if (m->m[x][0][z])
				{
					int d = x + z;
					if (d < dist)
					{
						dist = d;
						x0 = x;
						z0 = z;
					}
				}
		Assert(x0 != -1);
		b = Bot::Initial();
		cur.clear(R);
		dfs({ x0, 0, z0 });

		reach_cell(b, { 0, 0, 0 }, &cur, w, true);

		return 0;
	}

	int operator () (const Matrix *m, TraceWriter *w)
	{
		this->m = m;
		this->w = w;
		R = m->R;
		return solve();
	}

	const Matrix *m;
	TraceWriter *w;
	Matrix cur;
	int R;
	Bot *b;
};

int stupid_solver(const Matrix *src, const Matrix *target, TraceWriter *writer)
{
	if (src) exit(42);
	Assert(target);
	auto solver = new StupidSolver();
	int res = (*solver)(target, writer);
	delete solver;
	return res;
}

REG_SOLVER("dfs", stupid_solver);
