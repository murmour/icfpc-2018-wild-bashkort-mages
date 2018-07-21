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

struct Bot
{
	Point pos;
	int seeds;
	int id;

	static Bot Initial()
	{
		constexpr int initial_seeds = (1 << kMaxBots) - 2;
		return { {0, 0, 0}, initial_seeds, 1 };
	}
};

const int kMaxR = 250;

struct Matrix
{
	int R;
	char m[kMaxR][kMaxR][kMaxR];

	bool load_from_file(const char * filename);
	void clear(int r);

	char& operator [] (const Point &p)
	{
#ifdef DEBUG
		Assert(is_valid(p));
#endif
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
};

struct TraceWriter
{
	TraceWriter(const char *fname, int R);
	~TraceWriter();

	void halt();
	void wait();
	void flip();
	void move(const Point &from, const Point &to, bool reverse_order = false);
	void fusion_p(const Point &from, const Point &to);
	void fusion_s(const Point &from, const Point &to);
	void fill(const Point &from, const Point &to);
	void fission(const Point &from, const Point &to, int m);

	i64 get_energy() const { return energy; }
private:
	void next();

	FILE *f;
	bool high_harmonics = false;
	int n_bots = 1;
	int n_bots_next = 1; // number of bots in the next move
	int cur_bot = 0;
	i64 energy = 0; // total energy spent
	int R; // resolution
};

typedef std::function<int(const Matrix &target, TraceWriter &writer)> TSolverFun;

void RegisterSolver(const std::string id, TSolverFun f);
TSolverFun GetSolver(const std::string id);

#define REG_SOLVER(id, solver) \
	struct _R_##solver { _R_##solver() { RegisterSolver(id, solver); } } _r_##solver
