#include "common.h"

struct Point
{
	int x, y, z;
};

struct TraceWriter
{
	TraceWriter(const char *fname);
	~TraceWriter();

	void halt();
	void wait();
	void flip();
	void move(const Point &from, const Point &to, bool reverse_order);
	void fusion_p(const Point &from, const Point &to);
	void fusion_s(const Point &from, const Point &to);
	void fill(const Point &from, const Point &to);
	void fission(const Point &from, const Point &to, int m);
private:
	FILE *f;
};
