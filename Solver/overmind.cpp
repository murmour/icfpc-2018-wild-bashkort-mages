#include "trace_writer.h"
#include "system.h"

using namespace std;

vector<int> get_optimal_assignment(const vector<vector<int> > &a)
{
	const int inf = 1000000000;
	int n = (int)a.size();
	if (n == 0) return vector<int>();
	int m = (int)a[0].size();
	if (n > m)
	{
		vector<vector<int> > at(m, vector<int>(n));
		for (int i = 0; i < n; i++)
			for (int j = 0; j < m; j++)
				at[j][i] = a[i][j];
		auto rest = get_optimal_assignment(at);
		vector<int> res(n, -1);
		for (int i = 0; i < m; i++) res[rest[i]] = i;
		return res;
	}
	vector<int> u(n + 1), v(m + 1), p(m + 1), way(m + 1);
	vector<int> minv(m + 1);
	vector<char> used(m + 1);
	for (int i = 1; i <= n; ++i) {
		p[0] = i;
		int j0 = 0;
		fill(minv.begin(), minv.end(), inf);
		fill(used.begin(), used.end(), false);
		do
		{
			used[j0] = true;
			int i0 = p[j0], delta = inf, j1;
			for (int j = 1; j <= m; ++j)
				if (!used[j])
				{
					int cur = (i0 ? a[i0 - 1][j - 1] : 0) - u[i0] - v[j];
					if (cur < minv[j])
						minv[j] = cur, way[j] = j0;
					if (minv[j] < delta)
						delta = minv[j], j1 = j;
				}
			for (int j = 0; j <= m; ++j)
				if (used[j])
					u[p[j]] += delta, v[j] -= delta;
				else
					minv[j] -= delta;
			j0 = j1;
		} while (p[j0] != 0);
		do
		{
			int j1 = way[j0];
			p[j0] = p[j1];
			j0 = j1;
		} while (j0);
	}
	vector<int> res(n, -1);
	for (int j = 1; j <= m; ++j)
		if (p[j] > 0)
			res[p[j] - 1] = j - 1;
	return res;
}

struct OvermindSolver
{
	OvermindSolver(const Matrix *m, TraceWriter *w)
	{
		this->m = m;
		this->w = w;
		R = m->R;
	}

	vector<Point> get_corners(Point p, int s)
	{
		vector<Point> res;
		Point base = { p.x * s, p.y * s, p.z * s };
		res.push_back(base);
		res.push_back({ base.x + s - 1, base.y, base.z });
		res.push_back({ base.x + s - 1, base.y + s - 1, base.z });
		res.push_back({ base.x, base.y + s - 1, base.z });
		res.push_back({ base.x, base.y, base.z + s - 1 });
		res.push_back({ base.x + s - 1, base.y, base.z + s - 1 });
		res.push_back({ base.x + s - 1, base.y + s - 1, base.z + s - 1 });
		res.push_back({ base.x, base.y + s - 1, base.z + s - 1 });
		return res;
	}

	void fill_block(Point p, vector<Bot*> bots, int s)
	{
		Assert(bots.size() == 8);
		auto corners = get_corners(p, s);
		for (auto p : corners)
			Assert(cur.is_valid(p));
		// assign bots to corners
		vector<vector<int>> weights(8, vector<int>(8));
		for (int i = 0; i < 8; i++)
			for (int j = 0; j < 8; j++)
				weights[i][j] = bots[i]->pos.to(corners[j]).mlen();
		auto ass = get_optimal_assignment(weights);
		vector<Point> targets;
		for (int i = 0; i < 8; i++)
			targets.push_back(corners[ass[i]]);

		// mark future block cells as walls
		Region r = get_region(p, s);
		for (auto b : bots)
			Assert(!r.contains(b->pos));
		r.for_each([&](Point t) {
			cur[t] = true;
		});

		// bots go to their respective starting positions
		for (int i = 0; i < 8; i++)
			reach_cell(bots[i], targets[i], &cur, &bots[i]->mw);

		collect_commands(w, bots); // todo: waiting!

		// generate the block
		for (int i = 0; i < 8; i++)
			bots[i]->mw.g_fill(bots[i]->pos, targets[i], r.opposite(targets[i]) - targets[i]);

		collect_commands(w, bots);
	}

	void move_bots(vector<Bot*> bots, vector<Point> tgts)
	{
		Assert(bots.size() == tgts.size());
		int n = (int)bots.size();
		vector<vector<int>> weights(n, vector<int>(n));
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
				weights[i][j] = bots[i]->pos.to(tgts[j]).mlen();
		auto ass = get_optimal_assignment(weights);
		vector<Point> targets;
		for (int i = 0; i < n; i++)
			targets.push_back(tgts[ass[i]]);

		for (int i = 0; i < n; i++)
			reach_cell(bots[i], targets[i], &cur, &bots[i]->mw);

		collect_commands(w, bots); // todo: waiting!
	}

	void BFS_blocks(vector<Bot*> bots, Point p, int s)
	{
		queue<Point> q;

		temp_bfs.clear(R);
		auto push = [&](Point p) {
			if (temp_bfs[p]) return;
			q.push(p);
			temp_bfs[p] = true;
		};

		push(p);
		while (!q.empty())
		{
			auto t = q.front(); q.pop();

			fill_block(t, bots, s);

			for (auto d : kDeltas6)
			{
				auto a = t + d;
				if (!cur.is_valid(a, s)) continue;

				//if (a.x < XL || a.x > XR) continue;
				if (m->check_b(a, s))
				{
					push(a);
				}
			}
		}
	}

	void fission(vector<Bot*> active, int step)
	{
		// first, every active bot should get to its position
		int k = (int)active.size();
		for (int i = 0; i < k; i++)
			reach_cell(active[i], starts[active[i]->left], &cur, &active[i]->mw, true);
		collect_commands(w, active);
		// now, fission
		vector<Bot*> new_acitve;
		if (active.size() < starts.size())
		{
			for (int i = 0; i < k; i++)
			{
				auto b = active[i];
				new_acitve.push_back(b);
				if (b->seeds == 0)
					b->mw.wait();
				else
				{
					int low = low_bit(b->seeds);
					int high = high_bit(b->seeds);
					int have_seeds = high - low + 1;
					int give_to_child = (have_seeds + 1) / 2;
					int m = give_to_child - 1;

					int t = (b->left + b->right) / 2 + 1;
					Point pos = b->pos;
					pos.x++;
					bots[t] = new Bot(pos, make_seeds(low + 1, low + m), low);
					bots[t]->step = step;
					bots[t]->parent = b->left; // not id!
					bots[t]->left = t;
					bots[t]->right = b->right;
					b->right = t - 1;
					b->seeds = make_seeds(low + m + 1, high);

					b->mw.fission(b->pos, pos, give_to_child - 1);
					new_acitve.push_back(bots[t]);
				}
			}
		}
		collect_commands(w, active);
		if (!new_acitve.empty())
			fission(new_acitve, step + 1);
	}

	void fusion()
	{
		vector<Bot*> active;
		int max_step = 0;
		for (u32 i = 0; i < lims.size(); i++)
		{
			active.push_back(bots[i]);
			max_step = max(max_step, bots[i]->step);
		}
		while (active.size() > 1)
		{
			u32 k = active.size();
			bool f[kMaxBots] = { false };
			for (u32 i = 0; i < k; i++)
			{
				Bot *b = active[i];
				if (b->step == max_step)
				{
					// where to fusion?
					Point pos = bots[active[i]->parent]->pos;
					f[b->parent] = true;
					pos.x++;
					reach_cell(b, pos, &cur, &b->mw, true);
				}
			}
			collect_commands(w, active);
			vector<Bot*> new_active;
			for (u32 i = 0; i < k; i++)
			{
				Bot *b = active[i];
				if (b->step == max_step)
				{
					Point pos = b->pos;
					pos.x--;
					b->mw.fusion_s(b->pos, pos);
				}
				else
				{
					new_active.push_back(b);
					if (f[b->left])
					{
						Point pos = b->pos;
						pos.x++;
						b->mw.fusion_p(b->pos, pos);
					}
					else
					{
						b->mw.wait();
					}
				}
			}
			collect_commands(w, active);
			max_step--;
			Assert(new_active.size() < active.size());
			active.swap(new_active);
		}
	}

	int solve()
	{
		int s = 5; // side length
		const int NBots = 8;
		lims.clear();
		for (int i = 0; i < NBots; i++) lims.push_back(i);
		
		starts.clear();
		starts.push_back(Point::Origin);
		for (int i = 0; i + 1 < NBots; i++)
			starts.push_back({ lims[i] + 1, 0, 0 });
		lims0.clear();
		for (auto p : starts) lims0.push_back(p.x);
		cur.clear(R);

		// ok, now fission...
		memset(bots, 0, sizeof(bots));
		bots[0] = new Bot(Point::Origin, make_seeds(1, NBots - 1), 0);
		bots[0]->left = 0;
		bots[0]->right = NBots - 1;
		fission({ bots[0] }, 1);

		vector<Bot*> all_bots;
		for (int i = 0; i < NBots; i++) all_bots.push_back(bots[i]);

		int N = R / s;
		
		// initial position
		int x0 = -1, z0 = -1, y0 = -1;
		int dist = 1000;
		
		for (int x = 0; x < N; x++)
			for (int z = 0; z < N; z++)
				if (m->check_b({ x, 0, z }, s))
				{
					int d = x + z;
					if (d < dist)
					{
						dist = d;
						x0 = x;
						y0 = 0;
						z0 = z;
					}
				}
		
		Assert(x0 != -1);
	
		BFS_blocks(all_bots, { x0, y0, z0 }, s);		

		move_bots(all_bots, starts);
		
		collect_commands(w, all_bots);
		fusion();
		return 0;
	}

	vector<int> lims, lims0;
	vector<Point> starts;
	const Matrix *m;
	TraceWriter *w;
	Matrix cur;
	Matrix temp_bfs;
	int R, XL, XR;
	Bot *bots[kMaxBots];
};

int overmind_solver(const Matrix *src, const Matrix *target, TraceWriter *writer)
{
	if (src) exit(42);
	Assert(target);
	auto solver = new OvermindSolver(target, writer);
	int res = solver->solve();
	delete solver;
	return res;
}

REG_SOLVER("overmind", overmind_solver);
