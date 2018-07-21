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
	void moveto(Point p)
	{
		w->move(b.pos, p);
		b.pos = p;
	}

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
					moveto(a);
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
				moveto(a);
				continue;
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
				moveto(a);
				continue;
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
				moveto(a);
				continue;
			}
			bfs(b.pos, p, exact);
		}
	}

	void bfs(Point from, Point to, bool exact)
	{
		queue<Point> q;
		vector<Point> used;
		auto push = [&](Point t, char dir)
		{
			if (temp[t]) return;
			temp[t] = dir;
			q.push(t);
			used.push_back(t);
		};

		auto check = [&](Point t)
		{
			return exact ? t == to : t.is_near(to);
		};

		push(from, 10);
		while (!q.empty())
		{
			auto t = q.front(); q.pop();
			if (check(t))
			{
				// restore path
				vector<int> path;
				auto tt = t;
				while (t != from)
				{
					int dir = temp[t] - 1;
					Assert(dir >= 0 && dir < 6);
					path.push_back(dir);
					t = t - kDeltas6[dir];
				}
				reverse(path.begin(), path.end());
				int prev = -1, cnt = 0;
				for (auto d : path)
				{
					if (d == prev && cnt < 15)
						cnt++;
					else
					{
						if (cnt > 0) moveto(b.pos + kDeltas6[prev] * cnt);
						prev = d;
						cnt = 1;
					}
				}
				if (cnt > 0) moveto(b.pos + kDeltas6[prev] * cnt);
				Assert(b.pos == tt);

				// cleanup
				for (auto &p : used) temp[p] = 0;
				return;
			}
			for (int i = 0; i < 6; i++)
			{
				auto p = t + kDeltas6[i];
				if (!cur.is_valid(p)) continue;
				if (!cur[p])
					push(p, i + 1);
			}
		}
		Assert(false);
	}

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
			reach(t);
			w->fill(b.pos, t);
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
		temp.clear(R);
		BFS({ x0, 0, z0 });

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
	Matrix temp;
	Matrix temp_bfs;
	int R;
	Bot b;
};

int stupid2_solver(const Matrix &target, TraceWriter &writer)
{
	auto solver = new StupidSolver2();
	int res = (*solver)(target, writer);
	delete solver;
	return res;
}

REG_SOLVER("stupid3", stupid2_solver);
