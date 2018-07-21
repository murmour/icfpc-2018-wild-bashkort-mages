#include "trace_writer.h"

struct StupidSolver
{
	void reach(Point p, bool exact = false)
	{
		if (!exact && p == b.pos)
		{
			for (auto d : kDeltas6)
			{
				Point a = b.pos + d;
				if (!cur.is_valid(a)) continue;
				if (!cur[a])
				{
					w->move(b.pos, a);
					b.pos = a;
					return;
				}
			}
			Assert(false);
		}

		auto dir = b.pos.dir_to(p);
		while (exact ? b.pos != p : !b.pos.is_near(p))
		{
			// try x
			Point a = b.pos;
			int k = 0;
			while (a.x != p.x && k < 15)
			{
				Point t = a;
				t.x += dir.x;
				if (cur[t]) break;
				if (!exact && t == p) break;
				a = t;
				k++;
			}
			if (k > 0)
			{
				w->move(b.pos, a);
				b.pos = a;
				break;
			}

			// try y
			a = b.pos;
			k = 0;
			while (a.y != p.y && k < 15)
			{
				Point t = a;
				t.y += dir.y;
				if (cur[t]) break;
				if (!exact && t == p) break;
				a = t;
				k++;
			}
			if (k > 0)
			{
				w->move(b.pos, a);
				b.pos = a;
				break;
			}

			// try z
			a = b.pos;
			k = 0;
			while (a.z != p.z && k < 15)
			{
				Point t = a;
				t.z += dir.z;
				if (cur[t]) break;
				if (!exact && t == p) break;
				a = t;
				k++;
			}
			if (k > 0)
			{
				w->move(b.pos, a);
				b.pos = a;
				break;
			}
			Assert(false);
		}
	}

	void dfs(Point p)
	{
		reach(p);
		w->fill(b.pos, p);
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

		reach({ 0, 0, 0 }, true);

		w->halt();
		return 0;
	}

	int operator () (const Matrix &m, TraceWriter &w)
	{
		this->m = &m;
		this->w = &w;
		R = m.R;
		return solve();
	}

	const Matrix *m;
	TraceWriter *w;
	Matrix cur;
	int R;
	Bot b;
}

stupid_solver = StupidSolver();
REG_SOLVER("stupid", stupid_solver);