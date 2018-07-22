#pragma once

#include "common.h"
#include "zlib.h"


inline int sign(int x)
{
	if (x < 0) return -1;
	if (x == 0) return 0;
	return 1;
}

struct Point
{
	int x, y, z;

	Point to(const Point &other) const
	{
		return { other.x - x, other.y - y, other.z - z };
	}

	bool is_near(const Point &other) const
	{
		int a = Abs(x - other.x), b = Abs(y - other.y), c = Abs(z - other.z);
		return a <= 1 && b <= 1 && c <= 1 && a + b + c <= 2;
	}

	bool is_fd() const
	{
		return *this != Point::Origin && Abs(x) <= 30 && Abs(y) <= 30 && Abs(z) <= 30;
	}

	Point dir_to(const Point &other) const
	{
		return { sign(other.x - x), sign(other.y - y), sign(other.z - z) };
	}

	bool operator != (const Point &other) const
	{
		return x != other.x || y != other.y || z != other.z;
	}

	bool operator == (const Point &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}

	bool operator < (const Point &other) const
	{
		if (x != other.x) return x < other.x;
		if (y != other.y) return y < other.y;
		return z < other.z;
	}

	Point operator + (const Point &other) const
	{
		return { x + other.x, y + other.y, z + other.z };
	}

	Point operator * (int k) const
	{
		return { x * k, y * k, z * k };
	}

	Point operator - (const Point &other) const
	{
		return { x - other.x, y - other.y, z - other.z };
	}

	int n_diff(const Point &other) const
	{
		return (x != other.x) + (y != other.y) + (z != other.z);
	}

	static const Point Origin;
};

extern const Point kDeltas6[6];

const std::vector<Point>& Deltas26();

struct Region
{
	Region(const Point &pa, const Point &pb)
	{
		a.x = std::min(pa.x, pb.x);
		a.y = std::min(pa.y, pb.y);
		a.z = std::min(pa.z, pb.z);
		b.x = std::max(pa.x, pb.x);
		b.y = std::max(pa.y, pb.y);
		b.z = std::max(pa.z, pb.z);
	}

	bool operator < (const Region &other) const
	{
		if (a != other.a) return a < other.a;
		return b < other.b;
	}

	template<typename F> void for_each(F f) const
	{
		for (int x = a.x; x <= b.x; x++)
			for (int y = a.y; y <= b.y; y++)
				for (int z = a.z; z <= b.z; z++)
					f({ x, y, z });
	}

	int get_dim() const
	{
		return a.n_diff(b);
	}

	int get_bots() const
	{
		return 1 << get_dim();
	}

	Point a, b;
};

constexpr const int kMaxBots = 40;

const int kMaxR = 250;

struct Matrix
{
	int R;
	char m[kMaxR][kMaxR][kMaxR];
	int XL = -1, XR = -1;

	bool load_from_file(const char * filename);
	void clear(int r);

	char& operator [] (const Point &p)
	{
#ifdef DEBUG
		Assert(is_valid(p));
#endif
		return m[p.x][p.y][p.z];
	}

	bool get(const Point &p) const
	{
#ifdef DEBUG
		Assert(is_valid(p));
#endif
		if (XL != -1 && (p.x < XL || p.x > XR)) return true;
		return m[p.x][p.y][p.z];
	}

	char operator [] (const Point &p) const
	{
#ifdef DEBUG
		Assert(is_valid(p));
#endif
		return m[p.x][p.y][p.z];
	}

	bool is_valid(const Point &p) const
	{
		return p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x < R && p.y < R && p.z < R;
	}

	int get_filled_count() const
	{
		int res = 0;
		for (int x = 0; x < R; x++)
			for (int y = 0; y < R; y++)
				for (int z = 0; z < R; z++)
					if (m[x][y][z])
						res++;
		return res;
	}

	void set_x_limits(int x1, int x2)
	{
		XL = x1;
		XR = x2;
	}

	bool check_equal(const Matrix &other) const
	{
		if (R != other.R) return false;
		for (int x = 0; x < R; x++)
			for (int y = 0; y < R; y++)
				for (int z = 0; z < R; z++)
					if (bool(m[x][y][z]) != bool(other.m[x][y][z]))
						return false;
		return true;
	}
};

enum CommandType : u8
{
	cmdHalt,
	cmdWait,
	cmdFlip,
	cmdMove,
	cmdMoveR,
	cmdFusionP,
	cmdFusionS,
	cmdFill,
	cmdFission,
	cmdVoid,
	cmdGFill,
	cmdGVoid,
};

struct Command
{
	i8 dx, dy, dz;
	CommandType ty;
	i8 fdx = 0, fdy = 0, fdz = 0;
	i8 _unused = 0;
};

struct TraceWriter
{
	virtual void halt() = 0;
	virtual void wait() = 0;
	virtual void flip() = 0;
	virtual void move(const Point &from, const Point &to, bool reverse_order = false) = 0;
	virtual void fusion_p(const Point &from, const Point &to) = 0;
	virtual void fusion_s(const Point &from, const Point &to) = 0;
	virtual void fill(const Point &from, const Point &to) = 0;
	virtual void fission(const Point &from, const Point &to, int m) = 0;
	virtual Point do_command(const Point &p, Command cmd, int bot_id) = 0;

	virtual void void_(const Point &from, const Point &to) = 0;
	virtual void g_fill(const Point &from, const Point &to, const Point &fd) = 0;
	virtual void g_void(const Point &from, const Point &to, const Point &fd) = 0;
	virtual ~TraceWriter() {}
};

struct Bot;

struct MemoryTraceWriter : public TraceWriter
{
	MemoryTraceWriter(Bot *bot) : bot(bot) {}
	void halt();
	void wait();
	void flip();
	void move(const Point &from, const Point &to, bool reverse_order = false);
	void fusion_p(const Point &from, const Point &to);
	void fusion_s(const Point &from, const Point &to);
	void fill(const Point &from, const Point &to);
	void fission(const Point &from, const Point &to, int m);
	Point do_command(const Point &p, Command cmd, int bot_id);

	void void_(const Point &from, const Point &to);
	void g_fill(const Point &from, const Point &to, const Point &fd);
	void g_void(const Point &from, const Point &to, const Point &fd);

	std::vector<Command> commands;
	Bot *bot;
	Point p0; // bot's position before the sequence of commands

private:
	void add(const Command &cmd);
};

struct FileTraceWriter : public TraceWriter
{
	FileTraceWriter(const char *fname, int R, Matrix *src = nullptr);
	~FileTraceWriter();

	void halt();
	void wait();
	void flip();
	void move(const Point &from, const Point &to, bool reverse_order = false);
	void fusion_p(const Point &from, const Point &to);
	void fusion_s(const Point &from, const Point &to);
	void fill(const Point &from, const Point &to);
	void fission(const Point &from, const Point &to, int m);

	void void_(const Point &from, const Point &to);
	void g_fill(const Point &from, const Point &to, const Point &fd);
	void g_void(const Point &from, const Point &to, const Point &fd);

	Point do_command(const Point &p, Command cmd, int bot_id);
	i64 get_energy() const { return energy; }
	int get_filled_count() const { return n_filled; }
	const Matrix& get_matrix() const { return mat; }
private:
	void next();

	gzFile f;
	bool high_harmonics = false;
	int n_bots = 1;
	int n_bots_next = 1; // number of bots in the next move
	int cur_bot = 0;
	i64 energy = 0; // total energy spent
	int n_filled = 0;
	int R; // resolution
	Matrix mat;
	std::map<Region, int> gr_ops; // how many ops were for this region
};

struct Bot
{
	Point pos;
	i64 seeds;
	int id;
	int parent = -1;
	int step = 0;
	MemoryTraceWriter mw;
	int left = -1, right = -1;

	Bot() : pos(Point::Origin), seeds(0), id(-1), mw(this) {}

	Bot(Point pos, i64 seeds, int id) : pos(pos), seeds(seeds), id(id), mw(this)
	{
		parent = -1;
		step = 0;
	}

	static Bot* Initial()
	{
		constexpr i64 initial_seeds = (1ll << kMaxBots) - 2;
		return new Bot({ 0, 0, 0 }, initial_seeds, 0);
	}

	DISALLOW_COPY_AND_ASSIGN(Bot);
};

// returns the end point
Point reach_cell(Point from, Point to, const Matrix *env, TraceWriter *w, bool exact = false);

typedef std::function<int(const Matrix *src, const Matrix *target, TraceWriter *writer)> TSolverFun;

inline int high_bit(i64 seeds)
{
	for (int i = kMaxBots - 1; i >= 0; i--) if (seeds & (1ll << i)) return i;
	Assert(false);
	return -1;
}

inline int low_bit(i64 seeds)
{
	for (int i = 0; i < kMaxBots; i++) if (seeds & (1ll << i)) return i;
	Assert(false);
	return -1;
}

inline i64 make_seeds(int a, int b)
{
	return ((1ll << (b + 1)) - 1) ^ ((1ll << a) - 1);
}

// clears commands after collecting
void collect_commands(TraceWriter *w, const std::vector<Bot*> &bots);

void RegisterSolver(const std::string id, TSolverFun f);
TSolverFun GetSolver(const std::string id);

#define REG_SOLVER(id, solver) \
	struct _R_##solver { _R_##solver() { RegisterSolver(id, solver); } } _r_##solver
