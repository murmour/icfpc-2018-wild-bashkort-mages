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
		Assert(w->get_filled_count() % (s * s * s) == 0);
		Assert(bots.size() == 8);
		auto corners = get_corners(p, s);
		for (auto p : corners)
			Assert(cur.is_valid(p));
		Region r = get_region(p, s);

		int bad_corner = -1;
		int n_attempts = 0;
		vector<Point> targets;
		while (true)
		{
			n_attempts++;
			if (n_attempts > 5)
				Assert(false);
			// assign bots to corners
			vector<vector<int>> weights(8, vector<int>(8));
			for (int i = 0; i < 8; i++)
				for (int j = 0; j < 8; j++)
					weights[i][j] = Sqr(bots[i]->pos.to(corners[j]).mlen());

			auto ass = get_optimal_assignment(weights);
			targets.clear();
			for (int i = 0; i < 8; i++)
				targets.push_back(corners[ass[i]]);

			// bots go to their respective starting positions
			r.for_each([&](Point t) {
				bad[t] = true;
			});
			bad_corner = -1;
			//vector<Point> imm;
			//bool is_imm[8] = { 0 };

			for (int i = 0; i < 8; i++)
			{
				// checking for a bad corner:
				bool ok = false;
				for (auto d : kDeltas6)
				{
					auto t = targets[i] + d;
					if (!cur.is_valid(t)) continue;
					if (!bad[t] && !cur[t]) ok = true;
				}
				if (!ok)
				{
					Assert(bad_corner == -1);
					bad_corner = i;
				}
				else
				{
					if (n_attempts > 3)
					{
						for (int j = 0; j < 8; j++) if (i != j) cur[bots[j]->pos] = true;
						reach_cell(bots[i], targets[i], &cur, &bots[i]->mw, false, &bad);
						collect_commands(w, bots);
						for (int j = 0; j < 8; j++) if (i != j) cur[bots[j]->pos] = false;
					}
					else
					{
						reach_cell(bots[i], targets[i], &cur, &bots[i]->mw, false, &bad);
					}
				}
			}

			/*
			for (int i = 0; i < 8; i++)
			{
				if (i == bad_corner) continue;
				if (!is_imm[i])
					reach_cell(bots[i], targets[i], &cur, &bots[i]->mw, false, &bad);
			}

			for (int i = 0; i < 8; i++) if (is_imm[i])
			{
				cur[bots[i]->pos] = false;
			}
			*/

			r.for_each([&](Point t) {
				bad[t] = false;
			});
			if (bad_corner != -1)
			{
				reach_cell(bots[bad_corner], targets[bad_corner], &cur, &bots[bad_corner]->mw, true);
			}
			if (collect_commands_sync(w, bots)) break;
		}

		Point dir1;
		Point A, B2;
		Region rd(Point::Origin, Point::Origin);
		bool was_b2;
		int oc = -1;
		if (bad_corner != -1)
		{
			Point bc = targets[bad_corner];
			int dir1i = -1;
			for (int i = 0; i < 6; i++)
			{
				Point pt = bc + kDeltas6[i];
				if (cur.is_valid(pt) && !cur[pt])
				{
					dir1i = i;
					break;
				}
			}
			Assert(dir1i != -1);
			dir1 = kDeltas6[dir1i];
			// dir1 is a "good" direction
			oc = -1; // other corner
			for (int i = 0; i < 8; i++)
				if (targets[i] == bc + dir1 * (s - 1))
				{
					oc = i;
					break;
				}
			Assert(oc != -1);

			// dir2 is perpendicular to dir1 and contains a wall
			int dir2i = -1;
			for (int i = 0; i < 6; i++)
			{
				if ((dir1 + kDeltas6[i]).nz_count() != 2) continue;
				Point pt = bc + kDeltas6[i];
				if (cur.is_valid(pt) && cur[pt])
				{
					dir2i = i;
					break;
				}
			}
			Assert(dir2i != -1);
			auto dir2 = kDeltas6[dir2i];

			A = bc + dir2;
			Point B = targets[oc] + dir2;
			B2 = B + dir1;
			Assert(cur.is_valid(B2));
			if (!bots[oc]->pos.is_near(B2))
			{
				reach_cell(bots[oc], B2, &cur, &bots[oc]->mw, false);
				//Assert(bots[oc]->pos.is_near(targets[oc]));
				collect_commands(w, bots);
			}
			// remove the column
			bots[bad_corner]->mw.g_void(bots[bad_corner]->pos, A, B2 - A);
			bots[oc]->mw.g_void(bots[oc]->pos, B2, A - B2);
			collect_commands(w, bots);

			if (!bots[oc]->pos.is_near(targets[oc]))
			{
				reach_cell(bots[oc], targets[oc], &cur, &bots[oc]->mw, false);
				collect_commands(w, bots);
			}

			was_b2 = cur[B2];
			rd = Region(A, B2);
			rd.for_each([&](Point t) {
				cur[t] = false;
			});

			reach_cell(bots[bad_corner], A, &cur, &bots[bad_corner]->mw, true);

			Point exit_cell = targets[oc] + dir1;
			r.for_each([&](Point t) {
				bad[t] = true;
			});
			bad[exit_cell] = true;
			bad[B2] = true;
			bad[B] = true;
			bad[B - dir1] = true;
			reach_cell(bots[oc], targets[oc], &cur, &bots[oc]->mw, false, &bad);
			r.for_each([&](Point t) {
				bad[t] = false;
			});
			bad[exit_cell] = false;
			bad[B2] = false;
			bad[B] = false;
			bad[B - dir1] = false;

			collect_commands(w, bots);
		}

		// mark future block cells as walls
		for (auto b : bots)
			Assert(!r.contains(b->pos));
		r.for_each([&](Point t) {
			cur[t] = true;
		});

		// generate the block
		for (int i = 0; i < 8; i++)
			bots[i]->mw.g_fill(bots[i]->pos, targets[i], r.opposite(targets[i]) - targets[i]);

		collect_commands(w, bots);

		if (bad_corner != -1)
		{
			for (int i = 0; i < 8; i++) if (i != bad_corner)
				Assert(!rd.contains(bots[i]->pos));
			// bad corner bot rebuilds the column
			Assert(bots[bad_corner]->pos == A);
			auto t = A;
			for (int i = 0; i < s; i++)
			{
				reach_cell(bots[bad_corner], t + dir1, &cur, &bots[bad_corner]->mw, true);
				bots[bad_corner]->mw.fill(t + dir1, t);
				cur[t] = true;
				t = t + dir1;
			}
			Assert(bots[bad_corner]->pos == B2);
			if (was_b2)
			{
				reach_cell(bots[bad_corner], B2, &cur, &bots[bad_corner]->mw, false);
				bots[bad_corner]->mw.fill(bots[bad_corner]->pos, B2);
				cur[B2] = true;
				Assert(bots[bad_corner]->pos != bots[oc]->pos);
			}
			collect_commands(w, bots);
		}
		Assert(w->get_filled_count() % (s * s * s) == 0);
	}

	void move_bots(vector<Bot*> bots, vector<Point> tgts)
	{
		Assert(bots.size() == tgts.size());
		int n = (int)bots.size();

		for (int i = 0; i < n; i++)
			reach_cell(bots[i], tgts[i], &cur, &bots[i]->mw, true);

		if (collect_commands_sync(w, bots)) return;

		// fallback
		for (int i = 0; i < n; i++) if (bots[i]->pos != tgts[i])
		{
			for (int j = 0; j < n; j++) if (i != j) cur[bots[j]->pos] = true;
			reach_cell(bots[i], tgts[i], &cur, &bots[i]->mw, true);
			collect_commands(w, bots);
			for (int j = 0; j < n; j++) if (i != j) cur[bots[j]->pos] = false;
		}
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

			for (auto d : Deltas26())
			{
				auto a = t + d;
				if (!cur.is_valid(a, s)) continue;

				//if (a.x < XL || a.x > XR) continue;
				if (m->check_b(a, s))
				{
					if (check_for_all_subdeltas(d, [&](Point b) { return m->check_b(t + b, s); }))
						push(a);
					//push(a);
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
		bad.clear(R);

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
	Matrix bad;
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
