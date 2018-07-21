#include "common.h"

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

constexpr const int kMaxBots = 20;

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
	cmdFission
};

struct Command
{
	i8 dx, dy, dz;
	CommandType ty;
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
	virtual void do_command(Command cmd, int bot_id) = 0;
	virtual ~TraceWriter() {}
};

struct MemoryTraceWriter : public TraceWriter
{
	void halt();
	void wait();
	void flip();
	void move(const Point &from, const Point &to, bool reverse_order = false);
	void fusion_p(const Point &from, const Point &to);
	void fusion_s(const Point &from, const Point &to);
	void fill(const Point &from, const Point &to);
	void fission(const Point &from, const Point &to, int m);
	void do_command(Command cmd, int bot_id);

	std::vector<Command> commands;
};

struct FileTraceWriter : public TraceWriter
{
	FileTraceWriter(const char *fname, int R);
	~FileTraceWriter();

	void halt();
	void wait();
	void flip();
	void move(const Point &from, const Point &to, bool reverse_order = false);
	void fusion_p(const Point &from, const Point &to);
	void fusion_s(const Point &from, const Point &to);
	void fill(const Point &from, const Point &to);
	void fission(const Point &from, const Point &to, int m);

	void do_command(Command cmd, int bot_id);
	i64 get_energy() const { return energy; }
	int get_filled_count() const { return n_filled; }
private:
	void next();

	FILE *f;
	bool high_harmonics = false;
	int n_bots = 1;
	int n_bots_next = 1; // number of bots in the next move
	int cur_bot = 0;
	i64 energy = 0; // total energy spent
	int n_filled = 0;
	int R; // resolution
};

struct Bot
{
	Point pos;
	int seeds;
	int id;
	int parent = -1;
	int step = 0;
	MemoryTraceWriter mw;
	int left = -1, right = -1;

	Bot() : pos(Point::Origin), seeds(0), id(-1) {}

	Bot(Point pos, int seeds, int id) : pos(pos), seeds(seeds), id(id)
	{
		parent = -1;
		step = 0;
	}

	static Bot Initial()
	{
		constexpr int initial_seeds = (1 << kMaxBots) - 2;
		return Bot({ 0, 0, 0 }, initial_seeds, 0);
	}
};

// returns the end point
Point reach_cell(Point from, Point to, const Matrix *env, TraceWriter *w, bool exact = false);

typedef std::function<int(const Matrix *target, TraceWriter *writer)> TSolverFun;

inline int high_bit(int seeds)
{
	for (int i = kMaxBots - 1; i >= 0; i--) if (seeds & (1 << i)) return i;
	Assert(false);
	return -1;
}

inline int low_bit(int seeds)
{
	for (int i = 0; i < kMaxBots; i++) if (seeds & (1 << i)) return i;
	Assert(false);
	return -1;
}

inline int make_seeds(int a, int b)
{
	return ((1 << (b + 1)) - 1) ^ ((1 << a) - 1);
}

// clears commands after collecting
void collect_commands(TraceWriter *w, const std::vector<Bot*> &bots);

void RegisterSolver(const std::string id, TSolverFun f);
TSolverFun GetSolver(const std::string id);

#define REG_SOLVER(id, solver) \
	struct _R_##solver { _R_##solver() { RegisterSolver(id, solver); } } _r_##solver
