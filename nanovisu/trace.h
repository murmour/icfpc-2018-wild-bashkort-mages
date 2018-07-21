#ifndef __TRACE_H__
#define __TRACE_H__

#pragma comment(linker,"/STACK:64000000")
#define _CRT_SECURE_NO_WARNINGS

#include <vector>
#include <iostream>

#ifdef __linux__
#   define sprintf_s sprintf
#endif

using namespace std;

template<class T> inline T Abs(const T &x) { return x >= 0 ? x : -x; }

enum CommandType
{
	CT_HALT,
	CT_WAIT,
	CT_FLIP,
	CT_S_MOVE,
	CT_L_MOVE,
	CT_FUSION_P,
	CT_FUSION_S,
	CT_FISSION,
	CT_FILL,
	CT_UNDEFINED
};

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

struct TraceCommand
{
	CommandType tp = CT_UNDEFINED;
	Point p1 = { 0, 0, 0 }, p2 = { 0, 0, 0 };
	int m = 0;

	string cmd_to_string()
	{
		if (tp==CT_HALT) return "Halt";
		else if (tp==CT_WAIT) return "Wait";
		else if (tp==CT_FLIP) return "Flip";
		else if (tp==CT_S_MOVE)
		{
			char ch[50];
			sprintf_s( ch, "S Move [%d,%d,%d]", p1.x, p1.y, p1.z );
			return string(ch);
		}
		else if (tp==CT_L_MOVE)
		{
			char ch[50];
			sprintf_s( ch, "L Move [%d,%d,%d] [%d,%d,%d]", p1.x, p1.y, p1.z, p2.x, p2.y, p2.z );
			return string(ch);
		}
		else if (tp==CT_FUSION_P)
		{
			char ch[50];
			sprintf_s( ch, "Fusion P [%d,%d,%d]", p1.x, p1.y, p1.z );
			return string(ch);
		}
		else if (tp==CT_FUSION_S)
		{
			char ch[50];
			sprintf_s( ch, "Fusion S [%d,%d,%d]", p1.x, p1.y, p1.z );
			return string(ch);
		}
		else if (tp==CT_FISSION)
		{
			char ch[50];
			sprintf_s( ch, "Fission [%d,%d,%d] %d", p1.x, p1.y, p1.z, m );
			return string(ch);
		}
		else if (tp==CT_FILL)
		{
			char ch[50];
			sprintf_s( ch, "Fill [%d,%d,%d]", p1.x, p1.y, p1.z );
			return string(ch);
		}
		else if (tp==CT_UNDEFINED) return "UNDEFINED";
	}
};

struct TraceReader
{
	TraceReader();
	~TraceReader();

	bool open_file( const char *fname );
	TraceCommand read_next();
private:
	FILE *f;

	Point sld_to_point( int a, int i );
	Point lld_to_point( int a, int i );
	Point nd_to_point( int nd );
};

#endif
