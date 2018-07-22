#ifndef __TRACE_H__
#define __TRACE_H__

#pragma comment(linker,"/STACK:64000000")
#define _CRT_SECURE_NO_WARNINGS

#include <vector>
#include <iostream>
#include "zlib.h"

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
	CT_VOID,
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

	int bid = -1;

	string coord_to_string( Point p )
	{
		char ch[20];
		sprintf_s( ch, "[%d,%d,%d]", p.x, p.y, p.z );
		return string(ch);
	}

	string cmd_to_string( bool sh_bid, bool no, bool shrt, bool coords)
	{
		string re = "";
		if (sh_bid)
		{
			char ch[10];
			sprintf_s( ch, "%d", bid );
			re += string( ch );
		}
		string cc;
		if (tp==CT_HALT) cc = (shrt ? "Ha" : "Halt");
		else if (tp==CT_WAIT) cc = (shrt ? "Wa" : "Wait");
		else if (tp==CT_FLIP) cc = (shrt ? "Fl" : "Flip");
		else if (tp==CT_S_MOVE) cc = (shrt ? "SM" : "S Move");
		else if (tp==CT_L_MOVE) cc = (shrt ? "LM" : "L Move");
		else if (tp==CT_FUSION_P) cc = (shrt ? "FP" : "Fusion P");
		else if (tp==CT_FUSION_S) cc = (shrt ? "FS" : "Fusion S");
		else if (tp==CT_FISSION) cc = (shrt ? "Fi" : "Fission");
		else if (tp==CT_FILL) cc = (shrt ? "FL" : "Fill");
		else if (tp==CT_VOID) cc = (shrt ? "VD" : "Void");
		else if (tp==CT_UNDEFINED) cc = (shrt ? "UD" : "Undefined");

		if (!no)
		{
			if (re!="") re += " ";
			re += cc;
		}

		if (coords)
		{
			if (re!="") re += " ";
			if (tp==CT_S_MOVE) re += coord_to_string( p1 );
			else if (tp==CT_L_MOVE) re += coord_to_string( p1 ) + " " + coord_to_string( p2 );
			else if (tp==CT_FUSION_P) re += coord_to_string( p1 );
			else if (tp==CT_FUSION_S) re += coord_to_string( p1 );
			else if (tp==CT_FISSION)
			{
				char str[10];
				sprintf_s( str, "%d", m );
				re += coord_to_string( p1 ) + " " + string( str );
			}
			else if (tp==CT_FILL) re += coord_to_string( p1 );
			else if (tp==CT_VOID) re += coord_to_string( p1 );
		}

		return re;
	}
};

struct TraceReader
{
	TraceReader();
	~TraceReader();

	bool open_file( const char *fname );
	TraceCommand read_next();
private:
	gzFile f;

	Point sld_to_point( int a, int i );
	Point lld_to_point( int a, int i );
	Point nd_to_point( int nd );
};

#endif
