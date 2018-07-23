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
		//Assert(w->get_filled_count() % (s * s * s) == 0);
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
		//Assert(w->get_filled_count() % (s * s * s) == 0);
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

	int bfs_simple(const Point &p0, int s)
	{
		queue<Point> q;
		int res = 0;

		auto push = [&](Point p) {
			if (temp_bfs[p]) return;
			q.push(p);
			temp_bfs[p] = true;
			res++;
		};

		push(p0);
		while (!q.empty())
		{
			auto t = q.front(); q.pop();
			for (auto d : kDeltas6)
			{
				auto a = t + d;
				if (!cur.is_valid(a, s)) continue;
				if (bad[a])
					push(a);
			}
		}
		return res;
	}

	double logb(double x, double base)
	{
		return log(x) / log(base);
	}

	pair<double, Point> get_side_score(int s)
	{
		Point start;
		int N = R / s;
		if (N == 0) return { -1, start };
		int k = 0;
		bad.R = N;
		for (int x = 0; x < N; x++)
			for (int y = 0; y < N; y++)
				for (int z = 0; z < N; z++)
				{
					bool t = m->check_b({ x, y, z }, s);
					bad.m[x][y][z] = t;
					if (t) k++;
				}
		if (k == 0) return { -1, start };
		// find largest cc
		temp_bfs.clear(N);
		int max_cc = 0;
		for (int y = 0; y < N; y++)
			for (int x = 0; x < N; x++)
				for (int z = 0; z < N; z++) if (bad.m[x][y][z] && !temp_bfs.m[x][y][z])
				{
					int t = bfs_simple({ x, y, z }, s);
					if (t > max_cc)
					{
						max_cc = max(max_cc, t);
						start = { x, y, z };
					}
				}
		int vol = max_cc * s * s * s;
		double score = logb(vol, 10) / logb(max_cc + 1, 2);
		//fprintf(stderr, "s = %2d, cc = %9d, score = %9.3lf, vol = %9d\n", s, max_cc, score, vol);
		if (vol < 1000) return { -1, start };
		return { score, start };
	}

	vector<Point> get_path_to_ground(Point start, int s)
	{
		int N = R / s;
		get_side_score(s); // put s-matrix to bad
		temp_bfs.clear(N);
		bfs_simple(start, s); // temp_bfs is filled regions
		bad.clear(R);
		// put filled regions to bad
		for (int x = 0; x < N; x++)
			for (int y = 0; y < N; y++)
				for (int z = 0; z < N; z++)
					if (temp_bfs.m[x][y][z])
					{
						auto r = get_region({ x, y, z }, s);
						r.for_each([&](Point t) {
							bad[t] = true;
						});
					}

		queue<Point> q;

		auto push = [&](Point p, int dir) {
			if (temp_bfs[p]) return;
			q.push(p);
			temp_bfs[p] = dir;
		};

		temp_bfs.clear(R);
		for (int x = 0; x < R; x++)
			for (int y = 0; y < R; y++)
				for (int z = 0; z < R; z++)
					if (bad.m[x][y][z])
					{
						push({ x, y, z }, 10);
					}

		while (!q.empty())
		{
			auto t = q.front(); q.pop();
			if (t.y == 0)
			{
				vector<Point> res;
				Point cur = t;
				while (temp_bfs[cur] != 10)
				{
					res.push_back(cur);
					cur = cur - kDeltas6[temp_bfs[cur] - 1];
				}
				return res;
			}
			for (int di = 0; di < 6; di++)
			{
				auto d = kDeltas6[di];
				auto a = t + d;
				if (!cur.is_valid(a)) continue;
				if ((*m)[a])
					push(a, di + 1);
			}
		}

		Assert(false);
		return {};
	}

	int solve()
	{
		cur.clear(R);
		//int s = 4; // side length
		int s = -1;
		double best = 0;
		Point start;
		for (int i = 4; i < 30; i++)
		{
			auto t = get_side_score(i);
			if (t.first > best)
			{
				best = t.first;
				start = t.second;
				s = i;
			}
		}
		if (s == -1) return 1;

		const int NBots = 8;
		lims.clear();
		for (int i = 0; i < NBots; i++) lims.push_back(i);
		
		starts.clear();
		starts.push_back(Point::Origin);
		for (int i = 0; i + 1 < NBots; i++)
			starts.push_back({ lims[i] + 1, 0, 0 });
		lims0.clear();
		for (auto p : starts) lims0.push_back(p.x);
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
		Point start_p = get_corners(start, s)[0];
		int x0 = start_p.x;
		int y0 = start_p.y;
		int z0 = start_p.z;
		const int NB = 7;
		if (y0 > 0)
		{
			auto path = get_path_to_ground(start, s);
			for (int i = 0; i < 8; i++) if (i != NB) cur[bots[i]->pos] = true;
			for (Point t : path)
			{
				reach_cell(bots[NB], t, &cur, &bots[NB]->mw);
				bots[NB]->mw.fill(bots[NB]->pos, t);
				cur[t] = true;
			}
			collect_commands(w, all_bots);
			for (int i = 0; i < 8; i++) if (i != NB) cur[bots[i]->pos] = false;
		}
		BFS_blocks(all_bots, start, s);
		
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

struct CompleterSolver
{
	CompleterSolver(const Matrix *m, TraceWriter *w, bool new_search)
	{
		this->m = m;
		this->w = w;
		this->new_search = new_search;
		R = m->R;
	}

	void BFS(Bot *b, const vector<Point> &pts, TraceWriter *w)
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

		for (auto p : pts)
			push(p);
		while (!q.empty())
		{
			auto t = q.front(); q.pop();
			if (!cur[t])
			{
				if (new_search) {
					add_cell(t);
				}
				else {
					reach_cell(b, t, &cur, w);
					w->fill(b->pos, t);
				}
				cur[t] = true;
			}
			for (auto d : Deltas26())
			{
				auto a = t + d;
				if (!cur.is_valid(a)) continue;
				if (cur[a]) continue;
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
		temp_bfs.clear(R);
		cur.clear(R);
		for (int x = 0; x < R; x++)
			for (int y = 0; y < R; y++)
				for (int z = 0; z < R; z++)
				{
					Point p = { x, y, z };
					if (w->is_filled(p))
					{
						cur[p] = true;
						//temp_bfs[p] = true;
					}
				}

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
					if (m->m[x][y][z] && !cur.m[x][y][z]) weight[x]++;
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

		// ok, now fission...
		memset(bots, 0, sizeof(bots));
		bots[0] = new Bot(Point::Origin, make_seeds(1, n - 1), 0);
		bots[0]->left = 0;
		bots[0]->right = n - 1;
		fission({ bots[0] }, 1);
		//int need_cells = m->get_filled_count();
		vector<Point> pil_p;
		vector<Bot*> all_bots;
		for (int i = 0; i < n; i++) all_bots.push_back(bots[i]);

		auto run = [&](bool primary) -> bool
		{
			bool changed = false;
			for (int seg = 0; seg < n; seg++)
			{
				XL = lims0[seg];
				XR = lims[seg];
				cur.set_x_limits(XL, XR);
				// initial position
				int x0 = -1, z0 = -1, y0 = -1;
				int dist = 1000;
				vector<Point> pts;

				if (primary)
				{
					for (int x = XL; x <= XR; x++)
						for (int y = 0; y < R; y++)
							for (int z = 0; z < R; z++)
								if (cur.m[x][y][z])
									pts.push_back({ x, y, z });
					if (pts.empty())
					{
						for (int y = 0; y < R; y++)
						{
							for (int x = XL; x <= XR; x++)
								for (int z = 0; z < R; z++)
									if (m->m[x][y][z] && !cur.m[x][y][z])
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
						Point p0 = { x0, y0, z0 };
						pts.push_back(p0);
						if (y0 == 0)
						{
							// add other points
							for (int x = XL; x <= XR; x++)
								for (int z = 0; z < R; z++)
									if (m->m[x][0][z])
										pts.push_back({ x, 0, z });
						}
					}
					pil_p.push_back({ x0, y0, z0 });
					// todo: check if we have a filled neighbor??
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
				}
				else
				{
					// secondary run
					bool found = false;
					auto check = [&](Point p)
					{
						if (found) return;
						if (!(*m)[p]) return;
						if (cur[p]) return;
						if (p.y == 0)
						{
							found = true;
							pts.push_back(p);
							return;
						}
						for (auto d : kDeltas6)
						{
							auto t = p + d;
							if (!cur.is_valid(t)) continue;
							if (cur[t])
							{
								found = true;
								pts.push_back(p);
								return;
							}
						}
					};

					// check the floor
					for (int x = XL; x <= XR && !found; x++)
						for (int z = 0; z < R && !found; z++)
							check({ x, 0, z });
					// check the walls
					for (int y = 0; y < R && !found; y++)
						for (int z = 0; z < R && !found; z++)
						{
							check({ XL, y, z });
							check({ XR, y, z });
						}
				}
				if (!pts.empty())
					changed = true;
				else
					continue;

				BFS(bots[seg], pts, &bots[seg]->mw);
				if (y0 > 0)
				{
					reach_cell(bots[seg], { x0, y0 - 1, z0 }, &cur, &bots[seg]->mw);
				}
				else
					reach_cell(bots[seg], starts[seg], &cur, &bots[seg]->mw, true);
			}
			collect_commands(w, all_bots);
			return changed;
		};

		run(true);
		while (run(false));
		assert((int)pil_p.size() == n);

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

int overmind_solver(const Matrix *src, const Matrix *target, TraceWriter *writer)
{
	if (src) exit(42);
	Assert(target);
	auto solver = new OvermindSolver(target, writer);
	int res = solver->solve();
	delete solver;

	auto solver2 = new CompleterSolver(target, writer, false);
	res = solver2->solve();
	delete solver2;
	return res;
}

REG_SOLVER("overmind", overmind_solver);


struct CompleterSolverZ
{
	CompleterSolverZ(const Matrix *m, TraceWriter *w, bool new_search)
	{
		this->m = m;
		this->w = w;
		this->new_search = new_search;
		R = m->R;
	}

	void BFS(Bot *b, const vector<Point> &pts, TraceWriter *w)
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

		for (auto p : pts)
			push(p);
		while (!q.empty())
		{
			auto t = q.front(); q.pop();
			if (!cur[t])
			{
				if (new_search) {
					add_cell(t);
				}
				else {
					reach_cell(b, t, &cur, w);
					w->fill(b->pos, t);
				}
				cur[t] = true;
			}
			for (auto d : Deltas26())
			{
				auto a = t + d;
				if (!cur.is_valid(a)) continue;
				if (cur[a]) continue;
				if (a.z < ZL || a.z > ZR) continue;
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
					pos.z++; //z!
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
					pos.z++; //z!
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
					pos.z--; //z!
					b->mw.fusion_s(b->pos, pos);
				}
				else
				{
					new_active.push_back(b);
					if (f[b->left])
					{
						Point pos = b->pos;
						pos.z++; //z!
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
		temp_bfs.clear(R);
		cur.clear(R);
		for (int x = 0; x < R; x++)
			for (int y = 0; y < R; y++)
				for (int z = 0; z < R; z++)
				{
					Point p = { x, y, z };
					if (w->is_filled(p))
					{
						cur[p] = true;
						//temp_bfs[p] = true;
					}
				}

		int n = 2; // number of bots
		if (System::HasArg("bots"))
		{
			n = atoi(System::GetArgValue("bots").c_str());
			assert(n >= 1 && n <= kMaxBots);
		}
		int weight[kMaxR] = { 0 };
		int tot_w = 0;
		for (int z = 0; z < R; z++)
		{
			for (int y = 0; y < R; y++)
				for (int x = 0; x < R; x++)
					if (m->m[x][y][z] && !cur.m[x][y][z]) weight[z]++;
			tot_w += weight[z];
		}
		lims.clear();
		int cur_w = 0, cur_b = 1;
		for (int z = 0; z < R; z++)
		{
			cur_w += weight[z];
			if (cur_w >= ((i64)tot_w * cur_b) / n)
			{
				lims.push_back(z);
				cur_b++;
			}
		}
		Assert(lims.size() == n);
		starts.clear();
		starts.push_back(Point::Origin);
		for (int i = 0; i + 1 < n; i++)
			starts.push_back({ 0, 0, lims[i] + 1 });
		lims0.clear();
		for (auto p : starts) lims0.push_back(p.z);

		// ok, now fission...
		memset(bots, 0, sizeof(bots));
		bots[0] = new Bot(Point::Origin, make_seeds(1, n - 1), 0);
		bots[0]->left = 0;
		bots[0]->right = n - 1;
		fission({ bots[0] }, 1);
		//int need_cells = m->get_filled_count();
		vector<Point> pil_p;
		vector<Bot*> all_bots;
		for (int i = 0; i < n; i++) all_bots.push_back(bots[i]);

		auto run = [&](bool primary) -> bool
		{
			bool changed = false;
			for (int seg = 0; seg < n; seg++)
			{
				ZL = lims0[seg];
				ZR = lims[seg];
				cur.set_z_limits(ZL, ZR);
				// initial position
				int x0 = -1, z0 = -1, y0 = -1;
				int dist = 1000;
				vector<Point> pts;

				if (primary)
				{
					for (int x = 0; x < R; x++)
						for (int y = 0; y < R; y++)
							for (int z = ZL; z <= ZR; z++)
								if (cur.m[x][y][z])
									pts.push_back({ x, y, z });
					if (pts.empty())
					{
						for (int y = 0; y < R; y++)
						{
							for (int x = 0; x < R; x++)
								for (int z = ZL; z <= ZR; z++)
									if (m->m[x][y][z] && !cur.m[x][y][z])
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
						Point p0 = { x0, y0, z0 };
						pts.push_back(p0);
						if (y0 == 0)
						{
							// add other points
							for (int x = 0; x < R; x++)
								for (int z = ZL; z <= ZR; z++)
									if (m->m[x][0][z])
										pts.push_back({ x, 0, z });
						}
					}
					pil_p.push_back({ x0, y0, z0 });
					// todo: check if we have a filled neighbor??
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
				}
				else
				{
					// secondary run
					bool found = false;
					auto check = [&](Point p)
					{
						if (found) return;
						if (!(*m)[p]) return;
						if (cur[p]) return;
						if (p.y == 0)
						{
							found = true;
							pts.push_back(p);
							return;
						}
						for (auto d : kDeltas6)
						{
							auto t = p + d;
							if (!cur.is_valid(t)) continue;
							if (cur[t])
							{
								found = true;
								pts.push_back(p);
								return;
							}
						}
					};

					// check the floor
					for (int x = 0; x < R && !found; x++)
						for (int z = ZL; z <= ZR && !found; z++)
							check({ x, 0, z });
					// check the walls
					for (int x = 0; x < R && !found; x++)
						for (int y = 0; y < R && !found; y++)
						{
							check({ x, y, ZL });
							check({ x, y, ZR });
						}
				}
				if (!pts.empty())
					changed = true;
				else
					continue;

				BFS(bots[seg], pts, &bots[seg]->mw);
				if (y0 > 0)
				{
					reach_cell(bots[seg], { x0, y0 - 1, z0 }, &cur, &bots[seg]->mw);
				}
				else
					reach_cell(bots[seg], starts[seg], &cur, &bots[seg]->mw, true);
			}
			collect_commands(w, all_bots);
			return changed;
		};

		run(true);
		while (run(false));
		assert((int)pil_p.size() == n);

		// dismantle pillars
		for (int seg = 0; seg < n; seg++)
		{
			int x0 = pil_p[seg].x;
			int y0 = pil_p[seg].y;
			int z0 = pil_p[seg].z;

			if (y0 > 0)
			{
				ZL = lims0[seg];
				ZR = lims[seg];
				cur.set_z_limits(ZL, ZR);
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
		cur.set_z_limits(-1, -1);

		fusion();
		return 0;
	}

	vector<int> lims, lims0;
	vector<Point> starts;
	const Matrix *m;
	TraceWriter *w;
	Matrix cur;
	Matrix temp_bfs;
	int R, ZL, ZR;
	Bot *bots[kMaxBots];
	bool new_search;
};

int overmindz_solver(const Matrix *src, const Matrix *target, TraceWriter *writer)
{
	if (src) exit(42);
	Assert(target);
	auto solver = new OvermindSolver(target, writer);
	int res = solver->solve();
	delete solver;

	auto solver2 = new CompleterSolverZ(target, writer, false);
	res = solver2->solve();
	delete solver2;
	return res;
}

REG_SOLVER("overmindz", overmindz_solver);
