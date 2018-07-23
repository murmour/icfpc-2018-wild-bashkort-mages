#ifndef __TRACE_H__
#define __TRACE_H__

#pragma comment(linker,"/STACK:64000000")
#define _CRT_SECURE_NO_WARNINGS

#include <vector>
#include <iostream>
#include "zlib.h"

#include "../Solver/trace_writer.h"

#ifdef __linux__
#   define sprintf_s sprintf
#endif

using namespace std;

//template<class T> inline T Abs(const T &x) { return x >= 0 ? x : -x; }

enum RipCommandType
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
	CT_GFILL,
	CT_GVOID,
	CT_UNDEFINED
};

extern const Point kDeltas6[6];

struct TraceCommand
{
	RipCommandType tp = CT_UNDEFINED;
	Point p1 = { 0, 0, 0 }, p2 = { 0, 0, 0 };
	int m = 0;

	int bid = -1;
	int num = 0;

	string coord_to_string( Point p )
	{
		char ch[20];
		sprintf_s( ch, "[%d,%d,%d]", p.x, p.y, p.z );
		return string(ch);
	}

	string cmd_to_string( bool sh_bid, bool no, bool shrt, bool coords, bool num_f )
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
		else if (tp==CT_GFILL) cc = (shrt ? "GF" : "G Fill");
		else if (tp==CT_GVOID) cc = (shrt ? "GV" : "G Void");
		else if (tp==CT_UNDEFINED) cc = (shrt ? "UD" : "Undefined");

		if (!no)
		{
			if (re!="") re += " ";
			re += cc;
		}

		char str[20];
		if (coords)
		{
			if (re!="") re += " ";
			if (tp==CT_S_MOVE) re += coord_to_string( p1 );
			else if (tp==CT_L_MOVE) re += coord_to_string( p1 ) + " " + coord_to_string( p2 );
			else if (tp==CT_FUSION_P) re += coord_to_string( p1 );
			else if (tp==CT_FUSION_S) re += coord_to_string( p1 );
#ifdef __linux__
			else if (tp==CT_FISSION) re += coord_to_string( p1 ) + " " + std::to_string( m );
#else
			else if (tp == CT_FISSION) re += coord_to_string(p1) + " " + string(_itoa(m, str, 10));
#endif
			else if (tp==CT_FILL) re += coord_to_string( p1 );
			else if (tp==CT_VOID) re += coord_to_string( p1 );
			else if (tp==CT_GFILL) re += coord_to_string( p1 ) + " " + coord_to_string( p2 );
			else if (tp==CT_GVOID) re += coord_to_string( p1 ) + " " + coord_to_string( p2 );
		}

		if (num_f)
		{
			if (re!="") re += " ";
#ifdef __linux__
			re += std::to_string( num );
#else
			re += string(_itoa(num, str, 10));
#endif
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
