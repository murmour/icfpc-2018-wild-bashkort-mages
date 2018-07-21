#include "trace_writer.h"
#include "system.h"

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

struct CutterSolver
{
	CutterSolver(const Matrix *m, TraceWriter *w)
	{
		this->m = m;
		this->w = w;
		R = m->R;
	}

	void BFS(Bot *b, Point p, TraceWriter *w)
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
			b->pos = reach_cell(b->pos, t, &cur, w);
			w->fill(b->pos, t);
			cur[t] = true;
			for (auto d : Deltas26())
				//for (auto d : kDeltas6)
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
	}

	void fission(vector<Bot*> active, int step)
	{
		// first, every active bot should get to its position
		int k = (int)active.size();
		for (int i = 0; i < k; i++)
			active[i]->pos = reach_cell(active[i]->pos, starts[active[i]->left], &cur, &active[i]->mw, true);
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
					bots[t] = Bot(pos, make_seeds(low + 1, low + m), low);
					bots[t].step = step;
					bots[t].parent = b->left; // not id!
					bots[t].left = t;
					bots[t].right = b->right;
					b->right = t - 1;
					b->seeds = make_seeds(low + m + 1, high);

					b->mw.fission(b->pos, pos, give_to_child - 1);
					new_acitve.push_back(&bots[t]);
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
			active.push_back(&bots[i]);
			max_step = max(max_step, bots[i].step);
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
					Point pos = bots[active[i]->parent].pos;
					f[b->parent] = true;
					pos.x++;
					b->pos = reach_cell(b->pos, pos, &cur, &b->mw, true);
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
			assert(n >= 1 && n <= 20);
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

		// ok, now fission...
		bots[0] = Bot(Point::Origin, make_seeds(1, n-1), 0);
		bots[0].left = 0;
		bots[0].right = n - 1;
		fission({ &bots[0] }, 1);

		for (int seg = 0; seg < n; seg++)
		{
			XL = lims0[seg];
			XR = lims[seg];
			cur.set_x_limits(XL, XR);
			// initial position
			int x0 = -1, z0 = 1;
			int dist = 1000;
			for (int x = XL; x <= XR; x++)
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
			
			BFS(&bots[seg], { x0, 0, z0 }, &bots[seg].mw);

			bots[seg].pos = reach_cell(bots[seg].pos, starts[seg], &cur, &bots[seg].mw, true);
		}
		vector<Bot*> all_bots;
		for (int i = 0; i < n; i++) all_bots.push_back(bots + i);
		collect_commands(w, all_bots);
		cur.set_x_limits(-1, -1);
		fusion();
		w->halt();
		validate();
		return 0;
	}

	void validate()
	{
		for (int x = 0; x < R; x++)
			for (int y = 0; y < R; y++)
				for (int z = 0; z < R; z++)
					if (bool(m->m[x][y][z]) != bool(cur.m[x][y][z]))
						Assert(false);
	}

	vector<int> lims, lims0;
	vector<Point> starts;
	const Matrix *m;
	TraceWriter *w;
	Matrix cur;
	Matrix temp_bfs;
	int R, XL, XR;
	Bot bots[kMaxBots];
};

int cutter_solver(const Matrix *target, TraceWriter *writer)
{
	auto solver = new CutterSolver(target, writer);
	int res = solver->solve();
	delete solver;
	return res;
}

REG_SOLVER("cutter", cutter_solver);
