
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

FileTraceWriter::FileTraceWriter(const char * fname, int R) : R(R)
{
	f = gzopen(fname, "wb");
	energy = 3 * R * R * R + 20;
}

FileTraceWriter::~FileTraceWriter()
{
	gzclose(f);
}

void FileTraceWriter::next()
{
	Assert(n_bots > 0);
	cur_bot++;
	if (cur_bot == n_bots)
	{
		n_bots = n_bots_next;
		cur_bot = 0;
		if (n_bots > 0)
			energy += (high_harmonics ? 30 : 3) * R * R * R + 20 * n_bots;
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
	energy += 12;
	n_filled++;
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

void FileTraceWriter::do_command(Command cmd, int bot_id)
{
	Assert(bot_id == cur_bot);
	switch (cmd.ty)
	{
	case cmdHalt:
		halt();
		break;
	case cmdWait:
		wait();
		break;
	case cmdFlip:
		flip();
		break;
	case cmdMove:
		move(Point::Origin, { cmd.dx, cmd.dy, cmd.dz }, false);
		break;
	case cmdMoveR:
		move(Point::Origin, { cmd.dx, cmd.dy, cmd.dz }, true);
		break;
	case cmdFusionP:
		fusion_p(Point::Origin, { cmd.dx, cmd.dy, cmd.dz });
		break;
	case cmdFusionS:
		fusion_s(Point::Origin, { cmd.dx, cmd.dy, cmd.dz });
		break;
	case cmdFill:
		fill(Point::Origin, { cmd.dx, cmd.dy, cmd.dz });
		break;
	case cmdFission:
		fission(Point::Origin, { (cmd.dx & 3) - 1, cmd.dy, cmd.dz }, cmd.dx >> 2);
		break;
	default:
		Assert(false);
	}
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
				w->do_command(bots[j]->mw.commands[i], j);
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
	commands.push_back({ 0, 0, 0, cmdHalt });
}

void MemoryTraceWriter::wait()
{
	commands.push_back({ 0, 0, 0, cmdWait });
}

void MemoryTraceWriter::flip()
{
	commands.push_back({ 0, 0, 0, cmdFlip });
}

void MemoryTraceWriter::move(const Point & from, const Point & to, bool reverse_order)
{
	auto d = from.to(to);
	commands.push_back({ i8(d.x), i8(d.y), i8(d.z), reverse_order ? cmdMoveR : cmdMove });
}

void MemoryTraceWriter::fusion_p(const Point & from, const Point & to)
{
	auto d = from.to(to);
	commands.push_back({ i8(d.x), i8(d.y), i8(d.z), cmdFusionP });
}

void MemoryTraceWriter::fusion_s(const Point & from, const Point & to)
{
	auto d = from.to(to);
	commands.push_back({ i8(d.x), i8(d.y), i8(d.z), cmdFusionS });
}

void MemoryTraceWriter::fill(const Point & from, const Point & to)
{
	auto d = from.to(to);
	commands.push_back({ i8(d.x), i8(d.y), i8(d.z), cmdFill });
}

void MemoryTraceWriter::fission(const Point & from, const Point & to, int m)
{
	auto d = from.to(to);
	commands.push_back({ i8(d.x + 1 + (m << 2)), i8(d.y), i8(d.z), cmdFission });
}

void MemoryTraceWriter::do_command(Command cmd, int bot_id)
{
	commands.push_back(cmd);
}

static Point bfs_reach(Point from, Point to, const Matrix * env, TraceWriter *w, bool exact)
{
	Point P = from;

	auto moveto = [&](Point p)
	{
		w->move(P, p);
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
			int prev = -1, cnt = 0;
			for (auto d : path)
			{
				if (d == prev && cnt < 15)
					cnt++;
				else
				{
					if (cnt > 0) moveto(P + kDeltas6[prev] * cnt);
					prev = d;
					cnt = 1;
				}
			}
			if (cnt > 0) moveto(P + kDeltas6[prev] * cnt);
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


Point reach_cell(Point from, Point to, const Matrix * env, TraceWriter *w, bool exact)
{
	Point P = from;

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
		return bfs_reach(P, to, env, w, exact);
	}
	return P;
}
