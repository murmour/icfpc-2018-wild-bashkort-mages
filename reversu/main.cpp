#pragma comment (lib, "zdll.lib")

#pragma comment(linker,"/STACK:64000000")
#define _CRT_SECURE_NO_WARNINGS

#include "../nanovisu/trace.h"
#include "../Solver/trace_writer.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

void __never(int a){printf("\nOPS %d", a);}
#define ass(s) {if (!(s)) {__never(__LINE__);cout.flush();cerr.flush();abort();}}

vector< vector< TraceCommand > > trace_cmd;
vector< vector< TraceCommand > > rev_cmd;

//constexpr const int kMaxBots = 40;

struct RipBot
{
	Point pos;
	long long seeds;
	int id;
	bool active;

	/*static Bot Initial()
	{
		constexpr int initial_seeds = (1 << kMaxBots) - 2;
		return { {0, 0, 0}, initial_seeds, 1, true };
	}*/
};

void load_trace_file( string file )
{
	cerr << "loading trace file " << file.c_str() << "\n";

	TraceReader tr;
	if (! tr.open_file( (string("../data/tracesF/") + file).c_str() ) )
	{
		cerr << "cannot open " << file.c_str() << "\n";
		return;
	}
	trace_cmd.clear();

	vector< RipBot > bots_now;
	bots_now.push_back( { { 0, 0, 0 }, ((long long)1 << kMaxBots) - 2, 0, true } );
	vector< RipBot > bots_add;
	vector< RipBot > bots_rem;
	int cur_bot = 0;
	vector< TraceCommand > vec;
	while(1)
	{
		TraceCommand cmd = tr.read_next();
		cmd.bid = bots_now[cur_bot].id;
		if (cmd.tp == CT_UNDEFINED)
		{
			//cerr << "undefined command\n";
			break;
		}
		vec.push_back( cmd );

		if (cmd.tp == CT_S_MOVE)
		{
			bots_now[cur_bot].pos = bots_now[cur_bot].pos + cmd.p1;
		}
		else if (cmd.tp == CT_L_MOVE)
		{
			bots_now[cur_bot].pos = bots_now[cur_bot].pos + cmd.p1 + cmd.p2;
		}
		else if (cmd.tp == CT_FUSION_P)
		{
			for (int a=0; a<(int)bots_now.size(); a++)
				if (bots_now[a].pos == bots_now[cur_bot].pos + cmd.p1)
				{
					bots_now[cur_bot].seeds |= ( bots_now[a].seeds | ((long long)1 << bots_now[a].id) );
					bots_rem.push_back( bots_now[a] );
					break;
				}
		}
		else if (cmd.tp == CT_FISSION)
		{
			vector< int > v;
			for (int a=0; a<kMaxBots; a++)
				if ((bots_now[cur_bot].seeds>>a)&1)
					v.push_back( a );
			long long new_bot_mask = 0;
			for (int a=1; a<cmd.m+1; a++)
				new_bot_mask |= ((long long)1 << v[a]);
			bots_add.push_back( { bots_now[ cur_bot ].pos + cmd.p1, new_bot_mask, v[0], true } );
			bots_now[cur_bot].seeds = 0;
			for (int a=cmd.m+1; a<(int)v.size(); a++)
				bots_now[cur_bot].seeds |= ((long long)1 << v[a]);
		}

		cur_bot++;
		if (cur_bot==(int)bots_now.size())
		{
			cur_bot = 0;
			for (int a=0; a<(int)bots_add.size(); a++)
				bots_now.push_back( bots_add[a] );
			for (int a=0; a<(int)bots_rem.size(); a++)
			{
				vector< RipBot > tmp;
				for (int b=0; b<(int)bots_now.size(); b++)
					if (bots_now[b].id != bots_rem[a].id)
						tmp.push_back( bots_now[b] );
				bots_now = tmp;
			}
			bots_add.clear();
			bots_rem.clear();
			sort( bots_now.begin(), bots_now.end(),
				[](const RipBot & a, const RipBot & b) -> bool { return a.id < b.id; } );
			trace_cmd.push_back( vec );
			vec.clear();
		}
	}

	cerr << "ok! commands=" << (int)trace_cmd.size() << "\n";
}

int main(int argc, char * argv[])
{
	freopen( "output.txt", "w", stdout );

	load_trace_file( "FA1_cutterpillar11.nbt.gz" );

	vector< RipBot > bots_now;
	bots_now.push_back( { { 0, 0, 0 }, ((long long)1 << kMaxBots) - 2, 0, true } );
	vector< RipBot > bots_add;
	vector< RipBot > bots_rem;
	int cur_bot = 0;

	int sz = (int)trace_cmd.size();
	for (int i=0; i<sz-1; i++)
	{
		int sz2 = (int)trace_cmd[i].size();
		vector< TraceCommand > vec;
		for (int j=0; j<sz2; j++)
		{
			TraceCommand cmd = trace_cmd[i][j];
			if (cmd.tp == CT_HALT)
			{
				ass( false );
			}
			else if (cmd.tp == CT_WAIT)
			{
				vec.push_back( cmd );
			}
			else if (cmd.tp == CT_FLIP)
			{
				vec.push_back( cmd );
			}
			else if (cmd.tp == CT_S_MOVE)
			{
				bots_now[cur_bot].pos = bots_now[cur_bot].pos + cmd.p1;

				cmd.p1 = cmd.p1 * -1;
				vec.push_back( cmd );
			}
			else if (cmd.tp == CT_L_MOVE)
			{
				bots_now[cur_bot].pos = bots_now[cur_bot].pos + cmd.p1 + cmd.p2;

				swap( cmd.p1, cmd.p2 );
				cmd.p1 = cmd.p1 * -1;
				cmd.p2 = cmd.p2 * -1;
				vec.push_back( cmd );
			}
			else if (cmd.tp == CT_FUSION_P)
			{
				long long m1=0;
				for (int a=0; a<(int)bots_now.size(); a++)
					if (bots_now[a].pos == bots_now[cur_bot].pos + cmd.p1)
					{
						m1 = bots_now[a].seeds;
						bots_now[cur_bot].seeds |= ( bots_now[a].seeds | ((long long)1 << bots_now[a].id) );
						bots_rem.push_back( bots_now[a] );
						break;
					}

				cmd.tp = CT_FISSION;
				cmd.m = 0;
				for (int a=0; a<kMaxBots; a++)
					if ((m1 >> a)&1) cmd.m++;
				vec.push_back( cmd );
			}
			else if (cmd.tp == CT_FUSION_S)
			{
			}
			else if (cmd.tp == CT_FISSION)
			{
				vector< int > v;
				for (int a=0; a<kMaxBots; a++)
					if ((bots_now[cur_bot].seeds>>a)&1)
						v.push_back( a );
				long long new_bot_mask = 0;
				for (int a=1; a<cmd.m+1; a++)
					new_bot_mask |= ((long long)1 << v[a]);
				bots_add.push_back( { bots_now[ cur_bot ].pos + cmd.p1, new_bot_mask, v[0], true } );
				bots_now[cur_bot].seeds = 0;
				for (int a=cmd.m+1; a<(int)v.size(); a++)
					bots_now[cur_bot].seeds |= ((long long)1 << v[a]);

				cmd.tp = CT_FUSION_P;
				vec.push_back( cmd );
				cmd.tp = CT_FUSION_S;
				cmd.p1 = cmd.p1 * -1;
				cmd.bid = v[0];
				vec.push_back( cmd );
			}
			else if (cmd.tp == CT_FILL)
			{
				// cmd.tp = CT_VOID
				vec.push_back( cmd );
			}
			else if (cmd.tp == CT_VOID)
			{
				// cmd.tp = CT_FILL
				vec.push_back( cmd );
			}
			else if (cmd.tp == CT_UNDEFINED)
			{
				ass( false );
			}
			cur_bot++;
		}

		sort( vec.begin(), vec.end(),
			[](const TraceCommand & a, const TraceCommand & b) -> bool { return a.bid < b.bid; } );
		rev_cmd.push_back( vec );

		cur_bot = 0;
		for (int a=0; a<(int)bots_add.size(); a++)
			bots_now.push_back( bots_add[a] );
		for (int a=0; a<(int)bots_rem.size(); a++)
		{
			vector< RipBot > tmp;
			for (int b=0; b<(int)bots_now.size(); b++)
				if (bots_now[b].id != bots_rem[a].id)
					tmp.push_back( bots_now[b] );
			bots_now = tmp;
		}
		bots_add.clear();
		bots_rem.clear();
		sort( bots_now.begin(), bots_now.end(),
			[](const RipBot & a, const RipBot & b) -> bool { return a.id < b.id; } );
			
	}

	reverse( rev_cmd.begin(), rev_cmd.end() );

	for (int i=0; i<(int)rev_cmd.size(); i++)
	{
		for (int j=0; j<(int)rev_cmd[i].size(); j++)
			cout << rev_cmd[i][j].cmd_to_string( true, false, false, true ).c_str() << "; ";
		cout << "\n";
	}

	return 0;
}