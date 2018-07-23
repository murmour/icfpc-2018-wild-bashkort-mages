#include "trace_writer.h"
#include "system.h"

using namespace std;

struct CutterpillarSolver
{
	CutterpillarSolver(const Matrix *m, TraceWriter *w, bool new_search)
	{
		this->m = m;
		this->w = w;
		this->new_search = new_search;
		R = m->R;
	}

	void BFS(Bot *b, Point p, TraceWriter *w)
	{
		queue<Point> q;

		vector<Point> cells;
		auto add_cell = [&](Point t)
		{
			cells.push_back(t);
			if (cells.size() < 3) return;
			if (!b->pos.is_near(cells[0]))
			{
				int old_moves = w->get_n_moves();
				auto old_pos = b->pos;

				if (cells[0].to(cells[2]).mlen() <= 2)
				{
					reach_cell(b, cells[2], &cur, w);
					if (b->pos.is_near(cells[0]) && b->pos.is_near(cells[1]))
					{
						// great!
						w->fill(b->pos, cells[0]);
						cells.erase(cells.begin());
						return;
					}
					else
					{
						// backtrack
						Assert(w->backtrack(old_moves));
						b->pos = old_pos;
					}
				}
				if (cells[0].to(cells[1]).mlen() <= 2)
				{
					reach_cell(b, cells[1], &cur, w);
					if (b->pos.is_near(cells[0]))
					{
						// great!
						w->fill(b->pos, cells[0]);
						cells.erase(cells.begin());
						return;
					}
					else
					{
						// backtrack
						Assert(w->backtrack(old_moves));
						b->pos = old_pos;
					}
				}
			}
			// fallback
			reach_cell(b, cells[0], &cur, w);
			w->fill(b->pos, cells[0]);
			cells.erase(cells.begin());
		};

		auto push = [&](Point p) {
			if (temp_bfs[p]) return;
			q.push(p);
			temp_bfs[p] = true;
		};

		push(p);
		while (!q.empty())
		{
			auto t = q.front(); q.pop();
			if (new_search) {
				add_cell(t);
			} else {
				reach_cell(b, t, &cur, w);
				w->fill(b->pos, t);
			}
			cur[t] = true;
			for (auto d : Deltas26())
			{
				auto a = t + d;
				if (!cur.is_valid(a)) continue;
				if (a.x < XL || a.x > XR) continue;
				if ((*m)[a])
				{
					if (check_for_all_subdeltas(d, [&](Point b) { return (*m)[t + b]; }))
						push(a);
				}
			}
		}

		// process remaining cells
		for (auto t : cells)
		{
			reach_cell(b, t, &cur, w);
			w->fill(b->pos, t);
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
		int n = 2; // number of bots
		if (System::HasArg("bots"))
		{
			n = atoi(System::GetArgValue("bots").c_str());
			assert(n >= 1 && n <= kMaxBots);
		}
		int weight[kMaxR] = { 0 };
		int tot_w = 0;
		for (int x = 0; x < R; x++)
		{
			for (int y = 0; y < R; y++)
				for (int z = 0; z < R; z++)
					if (m->m[x][y][z]) weight[x]++;
			tot_w += weight[x];
		}
		lims.clear();
		int cur_w = 0, cur_b = 1;
		for (int x = 0; x < R; x++)
		{
			cur_w += weight[x];
			if (cur_w >= ((i64)tot_w * cur_b) / n)
			{
				lims.push_back(x);
				cur_b++;
			}
		}
		Assert(lims.size() == n);
		starts.clear();
		starts.push_back(Point::Origin);
		for (int i = 0; i + 1 < n; i++)
			starts.push_back({ lims[i] + 1, 0, 0 });
		lims0.clear();
		for (auto p : starts) lims0.push_back(p.x);
		cur.clear(R);
		temp_bfs.clear(R);

		// ok, now fission...
		memset(bots, 0, sizeof(bots));
		bots[0] = new Bot(Point::Origin, make_seeds(1, n - 1), 0);
		bots[0]->left = 0;
		bots[0]->right = n - 1;
		fission({ bots[0] }, 1);

		vector<Point> pil_p;
		for (int seg = 0; seg < n; seg++)
		{
			XL = lims0[seg];
			XR = lims[seg];
			cur.set_x_limits(XL, XR);
			// initial position
			int x0 = -1, z0 = -1, y0 = -1;
			int dist = 1000;
			for (int y = 0; y < R; y++)
			{
				for (int x = XL; x <= XR; x++)
					for (int z = 0; z < R; z++)
						if (m->m[x][y][z])
						{
							int d = x + z;
							if (d < dist)
							{
								dist = d;
								x0 = x;
								y0 = y;
								z0 = z;
							}
						}
				if (x0 != -1) break;
			}
			Assert(x0 != -1);

			pil_p.push_back({ x0, y0, z0 });
			if (y0 > 0)
			{
				// build a pillar
				for (int y = 0; y < y0; y++)
				{
					Point t = { x0, y, z0 };
					reach_cell(bots[seg], t, &cur, &bots[seg]->mw);
					bots[seg]->mw.fill(bots[seg]->pos, t);
					cur[t] = true;
				}
			}
			BFS(bots[seg], { x0, y0, z0 }, &bots[seg]->mw);
			if (y0 > 0)
			{
				reach_cell(bots[seg], { x0, y0 - 1, z0 }, &cur, &bots[seg]->mw);
			}
			else
				reach_cell(bots[seg], starts[seg], &cur, &bots[seg]->mw, true);
		}
		vector<Bot*> all_bots;
		for (int i = 0; i < n; i++) all_bots.push_back(bots[i]);
		collect_commands(w, all_bots);

		// dismantle pillars
		for (int seg = 0; seg < n; seg++)
		{
			int x0 = pil_p[seg].x;
			int y0 = pil_p[seg].y;
			int z0 = pil_p[seg].z;

			if (y0 > 0)
			{
				XL = lims0[seg];
				XR = lims[seg];
				cur.set_x_limits(XL, XR);
				// dismantle the pillar
				for (int y = y0 - 1; y >= 0; y--)
				{
					Point t = { x0, y, z0 };
					reach_cell(bots[seg], t, &cur, &bots[seg]->mw);
					bots[seg]->mw.void_(bots[seg]->pos, t);
					cur[t] = false;
				}
				reach_cell(bots[seg], starts[seg], &cur, &bots[seg]->mw, true);
			}
		}
		collect_commands(w, all_bots);
		cur.set_x_limits(-1, -1);
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
	bool new_search;
};

int cutterpillar_solver(const Matrix *src, const Matrix *target, TraceWriter *writer)
{
	if (src) exit(42);
	Assert(target);
	auto solver = new CutterpillarSolver(target, writer, true);
	int res = solver->solve();
	delete solver;
	return res;
}

int cutterpillar_solver_old(const Matrix *src, const Matrix *target, TraceWriter *writer)
{
	if (src) exit(42);
	Assert(target);
	auto solver = new CutterpillarSolver(target, writer, false);
	int res = solver->solve();
	delete solver;
	return res;
}

REG_SOLVER("cutterpillar", cutterpillar_solver);
REG_SOLVER("cutterpillarx", cutterpillar_solver_old);
