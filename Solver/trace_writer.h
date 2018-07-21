#include "common.h"

struct Point
{
	int x, y, z;
};

const int kMaxR = 250;

struct Matrix
{
	int R;
	bool m[kMaxR][kMaxR][kMaxR];

	bool load_from_file(const char * filename);
};

struct TraceWriter
{
	TraceWriter(const char *fname, int R);
	~TraceWriter();

	void halt();
	void wait();
	void flip();
	void move(const Point &from, const Point &to, bool reverse_order);
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