
#include "trace_writer.h"

#ifdef __linux__
#   define fread_s(b, blen, sz, count, stream) fread(b, sz, count, stream)
#endif

using namespace std;

const Point Point::Origin = { 0, 0, 0 };

const Point kDeltas6[6] = {
	{ -1, 0, 0 },
	{  1, 0, 0 },
	{  0,-1, 0 },
	{  0, 1, 0 },
	{  0, 0,-1 },
	{  0, 0, 1 },
};

FileTraceWriter::FileTraceWriter(const char * fname, int R, Matrix *src) : R(R)
{
	f = gzopen(fname, "wb");
	energy = 3 * R * R * R + 20;
	if (src)
	{
		Assert(R == src->R);
		mat.R = src->R;
		for (int i = 0; i < R; i++)
			for (int j = 0; j < R; j++)
				memcpy(mat.m[i][j], src->m[i][j], R);
	}
	else
	{
		mat.clear(R);
	}
}

FileTraceWriter::~FileTraceWriter()
{
	gzclose(f);
}

void FileTraceWriter::next()
{
	Assert(n_bots > 0);
	cur_bot++;
	n_moves++;
	if (cur_bot == n_bots)
	{
		n_bots = n_bots_next;
		cur_bot = 0;
		if (n_bots > 0)
			energy += (high_harmonics ? 30 : 3) * R * R * R + 20 * n_bots;
		if (!gr_ops.empty())
		{
			for (auto p : gr_ops)
				Assert(p.first.get_bots() == p.second);
			gr_ops.clear();
		}
	}
}

void FileTraceWriter::halt()
{
	u8 data = 255;
	gzwrite(f, &data, 1);
	Assert(!high_harmonics);
	Assert(n_bots == 1);
	n_bots_next--;
	next();
}

void FileTraceWriter::wait()
{
	u8 data = 254;
	gzwrite(f, &data, 1);
	next();
}

void FileTraceWriter::flip()
{
	u8 data = 253;
	gzwrite(f, &data, 1);
	high_harmonics = !high_harmonics;
	next();
}

void FileTraceWriter::move(const Point & from, const Point & to, bool reverse_order)
{
	auto write_long = [&](int from, int to, int q)
	{
		Assert(from != to);
		Assert(abs(from - to) <= 15);
		u8 data[2] = { u8((q << 4) + 4), u8(to - from + 15) };
		gzwrite(f, &data, 2);
		energy += 2 * abs(from - to);
		n_long_moves++;
	};
	auto write_short = [&](int from1, int to1, int q1, int from2, int to2, int q2)
	{
		Assert(abs(from1 - to1) <= 5);
		Assert(abs(from2 - to2) <= 5);
		if (reverse_order)
		{
			swap(from1, from2);
			swap(to1, to2);
			swap(q1, q2);
		}
		u8 data[2] = { u8((q2 << 6) + (q1 << 4) + 12), u8(((to2 - from2 + 5) << 4) + (to1 - from1 + 5)) };
		gzwrite(f, &data, 2);
		energy += 2 * (abs(from1 - to1) + 2 + abs(from2 - to2));
		n_short_moves++;
	};
	if (from.x == to.x && from.y == to.y)
		write_long(from.z, to.z, 3);
	else if (from.x == to.x && from.z == to.z)
		write_long(from.y, to.y, 2);
	else if (from.y == to.y && from.z == to.z)
		write_long(from.x, to.x, 1);
	else if (from.x == to.x)
		write_short(from.y, to.y, 2, from.z, to.z, 3);
	else if (from.y == to.y)
		write_short(from.x, to.x, 1, from.z, to.z, 3);
	else if (from.z == to.z)
		write_short(from.x, to.x, 1, from.y, to.y, 2);
	else
		Assert(false);
	next();
}

inline int get_nd(const Point &from, const Point &to)
{
	Assert(abs(from.x - to.x) + abs(from.y - to.y) + abs(from.z - to.z) <= 2);
	Assert(abs(from.x - to.x) <= 1 && abs(from.y - to.y) <= 1 && abs(from.z - from.z) <= 1);
	return (to.x - from.x + 1) * 9 + (to.y - from.y + 1) * 3 + to.z - from.z + 1;
}

void FileTraceWriter::fusion_p(const Point & from, const Point & to)
{
	u8 data = (get_nd(from, to) << 3) + 7;
	gzwrite(f, &data, 1);
	n_bots_next--;
	Assert(n_bots_next > 0);
	energy -= 24;
	next();
}

void FileTraceWriter::fusion_s(const Point & from, const Point & to)
{
	u8 data = (get_nd(from, to) << 3) + 6;
	gzwrite(f, &data, 1);
	next();
}

void FileTraceWriter::fill(const Point & from, const Point & to)
{
	u8 data = (get_nd(from, to) << 3) + 3;
	gzwrite(f, &data, 1);
	if (!mat[to])
	{
		energy += 12;
		mat[to] = true;
		n_filled++;
	}
	else
	{
		energy += 6;
	}
	next();
}

void FileTraceWriter::fission(const Point & from, const Point & to, int m)
{
	u8 data = (get_nd(from, to) << 3) + 5;
	gzwrite(f, &data, 1);
	Assert(m >= 0 && m <= 20);
	data = m;
	gzwrite(f, &data, 1);
	n_bots_next++;
	energy += 24;
	next();
}

void FileTraceWriter::void_(const Point &from, const Point &to)
{
	u8 data = (get_nd(from, to) << 3) + 2;
	gzwrite(f, &data, 1);
	if (mat[to])
	{
		energy -= 12;
		mat[to] = false;
		n_filled--;
	}
	else
	{
		energy += 3;
	}
	next();
}

void FileTraceWriter::g_fill(const Point &from, const Point &to, const Point & fd)
{
	u8 data[4];
	data[0] = (get_nd(from, to) << 3) + 1;
	Assert(fd.is_fd());
	data[1] = u8(fd.x + 30);
	data[2] = u8(fd.y + 30);
	data[3] = u8(fd.z + 30);
	gzwrite(f, &data, 4);
	Region r(to, to + fd);
	if (gr_ops.find(r) == gr_ops.end())
	{
		gr_ops[r] = 1;
		r.for_each([&](Point p) {
			if (!mat[p])
			{
				energy += 12;
				mat[p] = true;
				n_filled++;
			}
			else
			{
				energy += 6;
			}
		});
	}
	else
	{
		gr_ops[r]++;
	}
	next();
}

void FileTraceWriter::g_void(const Point &from, const Point &to, const Point & fd)
{
	u8 data[4];
	data[0] = (get_nd(from, to) << 3) + 0;
	Assert(fd.is_fd());
	data[1] = u8(fd.x + 30);
	data[2] = u8(fd.y + 30);
	data[3] = u8(fd.z + 30);
	gzwrite(f, &data, 4);
	Region r(to, to + fd);
	if (gr_ops.find(r) == gr_ops.end())
	{
		gr_ops[r] = 1;
		r.for_each([&](Point p) {
			if (mat[p])
			{
				energy -= 12;
				mat[p] = false;
				n_filled--;
			}
			else
			{
				energy += 3;
			}
		});
	}
	else
	{
		gr_ops[r]++;
	}
	next();
}

Point FileTraceWriter::do_command(const Point &p, Command cmd, int bot_id)
{
	Assert(bot_id == cur_bot);
	Point nd = Point({ cmd.dx, cmd.dy, cmd.dz });
	Point to = p + nd;
	switch (cmd.ty)
	{
	case cmdHalt:
		halt();
		return p;
	case cmdWait:
		wait();
		return p;
	case cmdFlip:
		flip();
		return p;
	case cmdMove:
		move(p, to, false);
		return to;
	case cmdMoveR:
		move(p, to, true);
		return to;
	case cmdFusionP:
		fusion_p(p, to);
		return p;
	case cmdFusionS:
		fusion_s(p, to);
		return p;
	case cmdFill:
		fill(p, to);
		return p;
	case cmdFission:
		fission(p, to, cmd.fdx);
		return p;
	case cmdVoid:
		void_(p, to);
		return p;
	case cmdGFill:
		g_fill(p, to, { cmd.fdx, cmd.fdy, cmd.fdz });
		return p;
	case cmdGVoid:
		g_void(p, to, { cmd.fdx, cmd.fdy, cmd.fdz });
		return p;
	default:
		Assert(false);
	}
	return Point::Origin;
}

bool Matrix::load_from_file(const char * filename)
{
	FILE *f = fopen(filename, "rb");
	if (!f) return false;

	unsigned char xr;
	const size_t sz = fread_s(&xr, 1, 1, 1, f);
	int r = xr;
	R = r;
	int i = 0, j = 0, k = 0;
	for (int a = 0; a<((r*r*r + 7) / 8); a++)
	{
		unsigned char z;
		const size_t sz = fread_s(&z, 1, 1, 1, f);
		for (int b = 0; b<8; b++)
		{
			m[i][j][k] = ((z >> b) & 1);
			k++;
			if (k == r) { k = 0; j++; }
			if (j == r) { j = 0; i++; }
			if (i == r) break;
		}
	}

	fclose(f);
	return true;
}

void Matrix::clear(int r)
{
	R = r;
	for (int i = 0; i < r; i++)
		for (int j = 0; j < r; j++)
			memset(m[i][j], 0, r);
}

void Matrix::init_sums()
{
	Assert(!sums);
	sums = new IntM();
	for (int x = 0; x < R; x++)
		for (int y = 0; y < R; y++)
			for (int z = 0; z < R; z++)
			{
				int r = bool(m[x][y][z]);
				if (x)
				{
					r += sums->m[x - 1][y][z];
					if (y)
					{
						r -= sums->m[x - 1][y - 1][z];
						if (z)
							r += sums->m[x - 1][y - 1][z - 1];
					}
					if (z)
					{
						r -= sums->m[x - 1][y][z - 1];
					}
				}
				if (y)
				{
					r += sums->m[x][y - 1][z];
					if (z)
					{
						r -= sums->m[x][y - 1][z - 1];
					}
				}
				if (z)
				{
					r += sums->m[x][y][z - 1];
				}
				sums->m[x][y][z] = r;
			}
}

map<string, TSolverFun> *solvers = nullptr;

const vector<Point>& Deltas26()
{
	static vector<Point> deltas;
	if (deltas.empty())
	{
		for (int d = 1; d <= 3; d++)
			for (int x = -1; x <= 1; x++)
				for (int y = -1; y <= 1; y++)
					for (int z = -1; z <= 1; z++)
						if (Abs(x) + Abs(y) + Abs(z) == d)
							deltas.push_back({ x, y, z });
	}
	return deltas;
}

void collect_commands(TraceWriter * w, const std::vector<Bot*> &bots_)
{
	u32 max_moves = 0;
	auto bots = bots_;
	sort(bots.begin(), bots.end(), [&](Bot *a, Bot *b) -> bool { return a->id < b->id; });
	u32 k = bots.size();
	for (auto b : bots) max_moves = max<u32>(max_moves, b->mw.commands.size());
	for (u32 i = 0; i < max_moves; i++)
		for (u32 j = 0; j < k; j++)
		{
			if (i >= bots[j]->mw.commands.size())
				w->wait();
			else
				bots[j]->mw.p0 = w->do_command(bots[j]->mw.p0, bots[j]->mw.commands[i], j);
		}
	for (auto b : bots) b->mw.commands.clear();
}

void RegisterSolver(const std::string id, TSolverFun f)
{
	if (!solvers)
		solvers = new map<string, TSolverFun>;
	Assert(solvers->find(id) == solvers->end());
	(*solvers)[id] = f;
}

TSolverFun GetSolver(const std::string id)
{
	if (!solvers)
		return nullptr;
	if (solvers->find(id) == solvers->end())
		return nullptr;
	return (*solvers)[id];
}

void MemoryTraceWriter::halt()
{
	add({ 0, 0, 0, cmdHalt });
}

void MemoryTraceWriter::wait()
{
	add({ 0, 0, 0, cmdWait });
}

void MemoryTraceWriter::flip()
{
	add({ 0, 0, 0, cmdFlip });
}

void MemoryTraceWriter::move(const Point & from, const Point & to, bool reverse_order)
{
	auto d = from.to(to);
	if (!commands.empty())
	{
		auto &prev = commands.back();
		if (prev.ty == cmdMove && d.mlen() <= 5 && d.nz_count() == 1)
		{
			Point dp = { prev.dx, prev.dy, prev.dz };
			if (dp.nz_count() == 1 && dp.mlen() <= 5)
			{
				Point s = d + dp;
				Assert(s != Point::Origin);
				prev = { i8(s.x), i8(s.y), i8(s.z), first_changed_coord(dp) > first_changed_coord(d) ? cmdMoveR : cmdMove };
				return;
			}
		}
	}
	add({ i8(d.x), i8(d.y), i8(d.z), reverse_order ? cmdMoveR : cmdMove });
}

void MemoryTraceWriter::fusion_p(const Point & from, const Point & to)
{
	auto d = from.to(to);
	add({ i8(d.x), i8(d.y), i8(d.z), cmdFusionP });
}

void MemoryTraceWriter::fusion_s(const Point & from, const Point & to)
{
	auto d = from.to(to);
	add({ i8(d.x), i8(d.y), i8(d.z), cmdFusionS });
}

void MemoryTraceWriter::fill(const Point & from, const Point & to)
{
	auto d = from.to(to);
	add({ i8(d.x), i8(d.y), i8(d.z), cmdFill });
}

void MemoryTraceWriter::void_(const Point & from, const Point & to)
{
	auto d = from.to(to);
	add({ i8(d.x), i8(d.y), i8(d.z), cmdVoid });
}

void MemoryTraceWriter::g_fill(const Point & from, const Point & to, const Point & fd)
{
	auto d = from.to(to);
	Command t({ i8(d.x), i8(d.y), i8(d.z), cmdGFill });
	t.fdx = i8(fd.x);
	t.fdy = i8(fd.y);
	t.fdz = i8(fd.z);
	add(t);
}

void MemoryTraceWriter::g_void(const Point & from, const Point & to, const Point & fd)
{
	auto d = from.to(to);
	Command t({ i8(d.x), i8(d.y), i8(d.z), cmdGVoid });
	t.fdx = i8(fd.x);
	t.fdy = i8(fd.y);
	t.fdz = i8(fd.z);
	add(t);
}

bool MemoryTraceWriter::backtrack(int old_moves_count)
{
	int n = (int)commands.size();
	Assert(old_moves_count >= 0 && old_moves_count <= n);
	while ((int)commands.size() > old_moves_count) commands.pop_back();
	return true;
}

void MemoryTraceWriter::add(const Command & cmd)
{
	if (commands.empty()) 
		p0 = bot->pos;
	commands.push_back(cmd);
}

void MemoryTraceWriter::fission(const Point & from, const Point & to, int m)
{
	auto d = from.to(to);
	Command t({ i8(d.x), i8(d.y), i8(d.z), cmdFission });
	t.fdx = i8(m);
	add(t);
}

Point MemoryTraceWriter::do_command(const Point &p, Command cmd, int bot_id)
{
	Assert(false);
	add(cmd);
	return p;
}

static Point bfs_reach(Point from, Point to, const Matrix * env, TraceWriter *w, bool exact)
{
	Point P = from;

	auto moveto = [&](Point p, bool reverse = false)
	{
		w->move(P, p, reverse);
		P = p;
	};

	static Matrix *temp_;
	if (!temp_)
	{
		temp_ = new Matrix();
		temp_->clear(env->R);
	}
	Matrix &temp = *temp_;

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
			vector<pair<int, int> > xpath;
			int prev = -1, cnt = 0;
			for (auto d : path)
			{
				if (d == prev && cnt < 15)
					cnt++;
				else
				{
					if (cnt > 0) xpath.push_back({prev, cnt});
					prev = d;
					cnt = 1;
				}
			}
			if (cnt > 0) xpath.push_back({ prev, cnt });

			int k = (int)xpath.size();
			for (int i = 0; i < k; i++)
				/*
				if (xpath[i].second <= 5 && i + 1 < k && xpath[i + 1].second <= 5)
				{
					auto a = xpath[i], b = xpath[i+1];
					moveto(P + kDeltas6[a.first] * a.second + kDeltas6[b.first] * b.second, need_reverse(a.first, b.first));
					i++;
				}
				else
				*/
				{
					moveto(P + kDeltas6[xpath[i].first] * xpath[i].second);
				}
			Assert(P == tt);

			// cleanup
			for (auto &p : used) temp[p] = 0;
			return P;
		}
		for (int i = 0; i < 6; i++)
		{
			auto p = t + kDeltas6[i];
			if (!env->is_valid(p)) continue;
			if (!env->get(p))
				push(p, i + 1);
		}
	}
	Assert(false);
	return from;
}

void reach_cell(Bot * b, Point to, const Matrix * env, TraceWriter * w, bool exact)
{
	b->pos = reach_cell(b->pos, to, env, w, exact);
}

Point reach_cell(Point from, Point to, const Matrix * env, TraceWriter *w, bool exact)
{
	Point P = from;
	int old_moves = w->get_n_moves();

	auto moveto = [&](Point p)
	{
		w->move(P, p);
		P = p;
	};

	std::vector<Command> res;
	if (!exact && from == to)
	{
		for (auto d : kDeltas6)
		{
			Point a = from + d;
			if (!env->is_valid(a)) continue;
			if (!env->get(a))
			{
				moveto(a);
				return P;
			}
		}
		Assert(false);
	}

	auto dir = from.dir_to(to);
	while (exact ? P != to : !P.is_near(to))
	{
		// try x
		Point a = P;
		int k = 0;
		while (a.x != to.x && k < 15)
		{
			Point t = a;
			t.x += dir.x;
			if (env->get(t)) break;
			if (!exact && t == to) break;
			a = t;
			k++;
		}
		if (k > 0)
		{
			moveto(a);
			continue;
		}

		// try y
		a = P;
		k = 0;
		while (a.y != to.y && k < 15)
		{
			Point t = a;
			t.y += dir.y;
			if (env->get(t)) break;
			if (!exact && t == to) break;
			a = t;
			k++;
		}
		if (k > 0)
		{
			moveto(a);
			continue;
		}

		// try z
		a = P;
		k = 0;
		while (a.z != to.z && k < 15)
		{
			Point t = a;
			t.z += dir.z;
			if (env->get(t)) break;
			if (!exact && t == to) break;
			a = t;
			k++;
		}
		if (k > 0)
		{
			moveto(a);
			continue;
		}
		if (w->backtrack(old_moves))
			return bfs_reach(from, to, env, w, exact);
		return bfs_reach(P, to, env, w, exact);
	}
	return P;
}
