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

TraceWriter::TraceWriter(const char * fname, int R) : R(R)
{
	f = fopen(fname, "wb");
	energy = 3 * R * R * R + 20;
}

TraceWriter::~TraceWriter()
{
	fclose(f);
}

void TraceWriter::next()
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

void TraceWriter::halt()
{
	u8 data = 255;
	fwrite(&data, 1, 1, f);
	Assert(!high_harmonics);
	Assert(n_bots == 1);
	n_bots_next--;
	next();
}

void TraceWriter::wait()
{
	u8 data = 254;
	fwrite(&data, 1, 1, f);
	next();
}

void TraceWriter::flip()
{
	u8 data = 253;
	fwrite(&data, 1, 1, f);
	high_harmonics = !high_harmonics;
	next();
}

void TraceWriter::move(const Point & from, const Point & to, bool reverse_order)
{
	auto write_long = [&](int from, int to, int q)
	{
		Assert(from != to);
		Assert(abs(from - to) <= 15);
		u8 data[2] = { (q << 4) + 4, to - from + 15 };
		fwrite(&data, 1, 2, f);
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
		u8 data[2] = { (q2 << 6) + (q1 << 4) + 12, ((to2 - from2 + 5) << 4) + (to1 - from1 + 5) };
		fwrite(&data, 1, 2, f);
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

void TraceWriter::fusion_p(const Point & from, const Point & to)
{
	u8 data = (get_nd(from, to) << 3) + 7;
	fwrite(&data, 1, 1, f);
	n_bots_next--;
	Assert(n_bots_next > 0);
	energy -= 24;
	next();
}

void TraceWriter::fusion_s(const Point & from, const Point & to)
{
	u8 data = (get_nd(from, to) << 3) + 6;
	fwrite(&data, 1, 1, f);
	next();
}

void TraceWriter::fill(const Point & from, const Point & to)
{
	u8 data = (get_nd(from, to) << 3) + 3;
	fwrite(&data, 1, 1, f);
	energy += 12;
	next();
}

void TraceWriter::fission(const Point & from, const Point & to, int m)
{
	u8 data = (get_nd(from, to) << 3) + 5;
	fwrite(&data, 1, 1, f);
	Assert(m >= 0 && m <= 20);
	data = m;
	fwrite(&data, 1, 1, f);
	n_bots_next++;
	energy += 24;
	next();
}

bool Matrix::load_from_file(const char * filename)
{
	FILE * f = fopen(filename, "rb");
	if (!f) return false;

	unsigned char xr;
	fread_s(&xr, 1, 1, 1, f);
	int r = xr;
	R = r;
	int i = 0, j = 0, k = 0;
	for (int a = 0; a<((r*r*r + 7) / 8); a++)
	{
		unsigned char z;
		fread_s(&z, 1, 1, 1, f);
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
