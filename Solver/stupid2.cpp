#include "trace_writer.h"

using namespace std;

template<typename F>
inline bool check_for_all_subdeltas(Point p, F f)
{
	if (p.x && !f({ p.x, 0, 0 })) return false;
	if (p.y && !f({ 0, p.y, 0 })) return false;
	if (p.z && !f({ 0, 0, p.z })) return false;
	if (p.x && p.y & !f({ p.x, p.y, 0 })) return false;
	if (p.x && p.z & !f({ p.x, 0, p.z })) return false;
	if (p.y && p.z & !f({ 0, p.y, p.z })) return false;
	return true;
}

struct StupidSolver2
{
	
	void BFS(Point p)
	{
		queue<Point> q;

		temp_bfs.clear(R);
		auto push = [&] (Point p) {
			if (temp_bfs[p]) return;
			q.push(p);
			temp_bfs[p] = true;
		};

		push(p);
		while (!q.empty())
		{
			auto t = q.front(); q.pop();

			b->pos = reach_cell(b->pos, t, &cur, w);
			w->fill(b->pos, t);
			cur[t] = true;
			for (auto d : Deltas26())
			//for (auto d : kDeltas6)
			{
				auto a = t + d;
				if (!cur.is_valid(a)) continue;
				if ((*m)[a])
				{
					if (check_for_all_subdeltas(d, [&](Point b) { return (*m)[t + b]; }))
						push(a);
				}
			}
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
		BFS({ x0, 0, z0 });

		b->pos = reach_cell(b->pos, { 0, 0, 0 }, &cur, w, true);

		w->halt();
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
	Matrix temp_bfs;
	int R;
	Bot *b;
};

int stupid2_solver(const Matrix *target, TraceWriter *writer)
{
	auto solver = new StupidSolver2();
	int res = (*solver)(target, writer);
	delete solver;
	return res;
}

REG_SOLVER("stupid3", stupid2_solver);
