
#include "trace.h"


TraceReader::TraceReader()
{
	f = 0;
}

TraceReader::~TraceReader()
{
	if (f) gzclose( f );
}

bool TraceReader::open_file( const char *fname )
{
	if (f) gzclose( f );
	f = gzopen(fname, "rb");
	if (!f) return false;
	return true;
}

Point TraceReader::sld_to_point( int a, int i )
{
	if (a==1) return { i-5, 0, 0 };
	else if (a==2) return { 0, i-5, 0 };
	else if (a==3) return { 0, 0, i-5 };
}

Point TraceReader::lld_to_point( int a, int i )
{
	if (a==1) return { i-15, 0, 0 };
	else if (a==2) return { 0, i-15, 0 };
	else if (a==3) return { 0, 0, i-15 };
}

Point TraceReader::nd_to_point( int nd )
{
	int dz = nd % 3 - 1;
	int dy = (nd/3) % 3 - 1;
	int dx = (nd/9) % 3 - 1;
	return { dx, dy, dz };
}

TraceCommand TraceReader::read_next()
{
	TraceCommand re;
	re.tp = CT_UNDEFINED;
	if (!f) return re;

	unsigned char ch = 0;
	const int sz = gzread(f, &ch, 1);
	//cerr << sz << " " << (int)ch << "\n";
	if (sz == 0) return re;
	int code = (ch&7);
	if (code==7)
	{
		if (ch==255) re.tp = CT_HALT;
		else
		{
			re.tp = CT_FUSION_P;
			re.p1 = nd_to_point( ch>>3 );
		}
	}
	else if (code==6)
	{
		if (ch==254) re.tp = CT_WAIT;
		else
		{
			re.tp = CT_FUSION_S;
			re.p1 = nd_to_point( ch>>3 );
		}
	}
	else if (code==5)
	{
		if (ch==253) re.tp = CT_FLIP;
		else
		{
			re.tp = CT_FISSION;
			re.p1 = nd_to_point( ch>>3 );
			unsigned char m;
			const int sz = gzread(f, &m, 1);
			re.m = m;
		}
	}
	else if (code==4)
	{
		unsigned char m;
		const int sz = gzread(f, &m, 1);
		if ((ch>>3)&1)
		{
			re.tp = CT_L_MOVE;
			re.p1 = sld_to_point( (ch>>4)&3, m&15 );
			re.p2 = sld_to_point( (ch>>6)&3, (m>>4)&15 );
		}
		else
		{
			re.tp = CT_S_MOVE;
			re.p1 = lld_to_point( (ch>>4)&3, m&31 );
		}
	}
	else if (code==3)
	{
		re.tp = CT_FILL;
		re.p1 = nd_to_point( ch>>3 );
	}
	else if (code==2)
	{
		re.tp = CT_VOID;
		re.p1 = nd_to_point( ch>>3 );
	}
	else if (code==1)
	{
		re.tp = CT_GFILL;
		re.p1 = nd_to_point( ch>>3 );
		unsigned char x, y, z;
		int sz;
		sz = gzread(f, &x, 1);
		sz = gzread(f, &y, 1);
		sz = gzread(f, &z, 1);
		re.p2 = { x-30, y-30, z-30 };
	}
	else if (code==0)
	{
		re.tp = CT_GVOID;
		re.p1 = nd_to_point( ch>>3 );
		unsigned char x, y, z;
		int sz;
		sz = gzread(f, &x, 1);
		sz = gzread(f, &y, 1);
		sz = gzread(f, &z, 1);
		re.p2 = { x-30, y-30, z-30 };
	}

	return re;
}
