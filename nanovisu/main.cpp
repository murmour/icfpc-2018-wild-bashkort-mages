#pragma comment (lib, "glu32.lib")
#pragma comment (lib, "opengl32.lib")

#pragma comment(linker,"/STACK:64000000")
#define _CRT_SECURE_NO_WARNINGS

#include "imgui.h"
#include "imgui_impl_freeglut.h"
#include "imgui_impl_opengl2.h"
#include "GL/freeglut.h"

#include "trace.h"

#ifdef __linux__
#   include <dirent.h>
#   include <unistd.h>
#   define Sleep(ms) usleep(ms * 1000)
#   define fread_s(b, blen, sz, count, stream) fread(b, sz, count, stream)
#   define sscanf_s sscanf
#else
#   include "dirent.h"
#endif

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

double colors[][3] = {
	{ 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 1.0, 0.0 },
	{ 1.0, 0.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 0.5, 0.5, 0.5 }, { 1.0, 0.5, 0.0 },
	{ 1.0, 1.0, 1.0 }
};

struct P
{
	double x, y, z;
	P( double _x=0., double _y=0., double _z=0. )
	{
		x=_x; y=_y; z=_z;
	}
};

struct VOXMAT
{
	int R;
	int filled;
	bool m[250][250][250];
};

double SIN[25], COS[25];
double pi;

void calc_sin_cos()
{
	pi = acos( -1. );
	for (int i=0; i<=20; i++)
	{
		SIN[i] = sin( pi*i/10 );
		COS[i] = cos( pi*i/10 );
	}
}

VOXMAT vm;

void sol()
{
	int r = 50;
	vm.R = r;
	vm.filled = 0;
	for (int a=0; a<r; a++)
		for (int b=0; b<r; b++)
			for (int c=0; c<r; c++)
			{
				int dx = (a+a-r)*(a+a-r);
				int dy = (b+b-r)*(b+b-r);
				int dz = (c+c-r)*(c+c-r);
				bool flag = ( dx+dy+dz < r*r );
				if (dx+dy < r*r*0.25 || dx+dz < r*r*0.25 || dy+dz < r*r*0.25)
					flag = false;
				vm.m[a][b][c] = flag;
				if (flag) vm.filled++;
			}

	calc_sin_cos();
}

void draw_sphere( double x, double y, double z, double r, int c )
{
	glBegin(GL_QUADS);
	for (int i=0; i<20; i++)
		for (int j=0; j<10; j++)
		{
			glColor3d( colors[c][0], colors[c][1], colors[c][2] );
			glVertex3d( x+SIN[i]*SIN[j]*r, y+COS[i]*SIN[j]*r, z+COS[j]*r );
			glColor3d( colors[c][0], colors[c][1], colors[c][2] );
			glVertex3d( x+SIN[i+1]*SIN[j]*r, y+COS[i+1]*SIN[j]*r, z+COS[j]*r );
			glColor3d( colors[c][0], colors[c][1], colors[c][2] );
			glVertex3d( x+SIN[i+1]*SIN[j+1]*r, y+COS[i+1]*SIN[j+1]*r, z+COS[j+1]*r );
			glColor3d( colors[c][0], colors[c][1], colors[c][2] );
			glVertex3d( x+SIN[i]*SIN[j+1]*r, y+COS[i]*SIN[j+1]*r, z+COS[j+1]*r );
		}
	glEnd();
}

void draw_cube( double x, double y, double z, double r, int c1, int c2, bool t1, bool t2, bool t3, bool t4, bool t5, bool t6 )
{
	double mul = 1.;
	double c1r = colors[c1][0], c1g = colors[c1][1], c1b = colors[c1][2];
	double c2r = colors[c2][0], c2g = colors[c2][1], c2b = colors[c2][2];

	if (t1)
	{
		mul = 1.0;
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x, y, z );
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x+r, y, z );
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x+r, y+r, z );
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x, y+r, z );
	}

	if (t2)
	{
		mul = 0.5;
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x, y, z );
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x, y+r, z );
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x, y+r, z+r );
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x, y, z+r );
	}

	if (t3)
	{
		mul = 0.8;
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x, y, z );
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x, y, z+r );
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x+r, y, z+r );
		glColor3d( c1r*mul, c1g*mul, c1b*mul );
		glVertex3d( x+r, y, z );
	}

	if (t4)
	{
		mul = 0.5;
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x+r, y+r, z+r );
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x, y+r, z+r );
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x, y, z+r );
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x+r, y, z+r );
	}

	if (t5)
	{
		mul = 0.25;
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x+r, y+r, z+r );
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x+r, y, z+r );
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x+r, y, z );
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x+r, y+r, z );
	}

	if (t6)
	{
		mul = 0.4;
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x+r, y+r, z+r );
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x+r, y+r, z );
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x, y+r, z );
		glColor3d( c2r*mul, c2g*mul, c2b*mul );
		glVertex3d( x, y+r, z+r );
	}
}

void draw_line( P p1, P p2, int c1, int c2, double width )
{
	if (width < 0.01) return;
	glLineWidth((float)width);
	glBegin(GL_LINES);
	glColor3d( colors[c1][0], colors[c1][1], colors[c1][2] );
	glVertex3d( p1.x, p1.y, p1.z );
	glColor3d( colors[c2][0], colors[c2][1], colors[c2][2] );
	glVertex3d( p2.x, p2.y, p2.z );
	glEnd();
}

void draw_voxel_matrix( VOXMAT & vm )
{
	draw_line( P( -0.5, -0.5, -0.5 ), P( -0.5, -0.5,  0.5 ), 1, 1, 2 );
	draw_line( P( -0.5, -0.5, -0.5 ), P(  0.5, -0.5, -0.5 ), 1, 1, 2 );
	draw_line( P(  0.5, -0.5,  0.5 ), P( -0.5, -0.5,  0.5 ), 1, 1, 2 );
	draw_line( P(  0.5, -0.5,  0.5 ), P(  0.5, -0.5, -0.5 ), 1, 1, 2 );

	draw_line( P( -0.5, -0.5, -0.5 ), P( -0.5,  0.5, -0.5 ), 1, 3, 1 );
	draw_line( P( -0.5, -0.5,  0.5 ), P( -0.5,  0.5,  0.5 ), 1, 3, 1 );
	draw_line( P(  0.5, -0.5, -0.5 ), P(  0.5,  0.5, -0.5 ), 1, 3, 1 );
	draw_line( P(  0.5, -0.5,  0.5 ), P(  0.5,  0.5,  0.5 ), 1, 3, 1 );

	draw_line( P( -0.5,  0.5, -0.5 ), P( -0.5,  0.5,  0.5 ), 3, 3, 2 );
	draw_line( P( -0.5,  0.5, -0.5 ), P(  0.5,  0.5, -0.5 ), 3, 3, 2 );
	draw_line( P(  0.5,  0.5,  0.5 ), P( -0.5,  0.5,  0.5 ), 3, 3, 2 );
	draw_line( P(  0.5,  0.5,  0.5 ), P(  0.5,  0.5, -0.5 ), 3, 3, 2 );

	double dd = 1. / vm.R;
	glBegin(GL_QUADS);
	for (int a=0; a<vm.R; a++)
		for (int b=0; b<vm.R; b++)
			for (int c=0; c<vm.R; c++)
				if (vm.m[a][b][c])
				{
					bool t1 = (c==0 || !vm.m[a][b][c-1]);
					bool t2 = (a==0 || !vm.m[a-1][b][c]);
					bool t3 = (b==0 || !vm.m[a][b-1][c]);
					bool t4 = (c==vm.R-1 || !vm.m[a][b][c+1]);
					bool t5 = (a==vm.R-1 || !vm.m[a+1][b][c]);
					bool t6 = (b==vm.R-1 || !vm.m[a][b+1][c]);
					draw_cube( a*dd-0.5, b*dd-0.5, c*dd-0.5, dd, 0, 2, t1, t2, t3, t4, t5, t6 );
				}
	glEnd();
}

static bool show_demo_window = false;
static bool show_another_window = false;
static ImVec4 clear_color = ImVec4(0.15f, 0.25f, 0.30f, 1.00f);

void my_display_code()
{
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()!
	// You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}

vector< string > model_files;
string cur_model = "not selected";
vector< pair< string, string > > trace_files;
string cur_trace = "not selected";
vector< TraceCommand > trace_cmd;
int cur_cmd;
bool trace_mode = false;
int trace_speed = 1;

constexpr const int kMaxBots = 20;

struct Bot
{
	Point pos;
	int seeds;
	int id;
	bool active;

	/*static Bot Initial()
	{
		constexpr int initial_seeds = (1 << kMaxBots) - 2;
		return { {0, 0, 0}, initial_seeds, 1, true };
	}*/
};

struct SYSTEM_STATE
{
	VOXMAT ss_vm;

	long long energy;
	Bot bots[kMaxBots];
	int cur_bot;
	int n_bots;

	void reset()
	{
		ss_vm.R = vm.R;
		int R = vm.R;
		ss_vm.filled = 0;
		for (int a=0; a<R; a++)
			for (int b=0; b<R; b++)
				for (int c=0; c<R; c++)
					ss_vm.m[a][b][c] = false;
		cur_bot = -1;
		for (int a=0; a<kMaxBots; a++)
			bots[a] = { { 0, 0, 0 }, (1 << kMaxBots) - 2, a+1, a==0 };
		n_bots = 1;
	}

	void perform_command( TraceCommand cmd )
	{
		while(1)
		{
			cur_bot++;
			if (cur_bot == kMaxBots)
			{
				cur_bot=0;
				energy += ss_vm.R * ss_vm.R * ss_vm.R * 3 + n_bots * 20;
			}
			if (bots[cur_bot].active) break;
		}

		if (cmd.tp == CT_HALT)
		{
			for (int a=0; a<kMaxBots; a++)
				bots[a].active = false;
		}
		else if (cmd.tp == CT_WAIT)
		{
		}
		else if (cmd.tp == CT_FLIP)
		{
		}
		else if (cmd.tp == CT_S_MOVE)
		{
			bots[cur_bot].pos = bots[cur_bot].pos + cmd.p1;
		}
		else if (cmd.tp == CT_L_MOVE)
		{
			bots[cur_bot].pos = bots[cur_bot].pos + cmd.p1 + cmd.p2;
		}
		else if (cmd.tp == CT_FUSION_P)
		{
		}
		else if (cmd.tp == CT_FUSION_S)
		{
		}
		else if (cmd.tp == CT_FISSION)
		{
		}
		else if (cmd.tp == CT_FILL)
		{
			ss_vm.m[ bots[cur_bot].pos.x + cmd.p1.x ][ bots[cur_bot].pos.y + cmd.p1.y ][ bots[cur_bot].pos.z + cmd.p1.z ] = true;
		}
	}
} ss;

void refresh_list_of_model_files()
{
	model_files.clear();
	cerr << "refresh list of model files\n";

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir("../data/problemsF/")) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
			if (ent->d_name[0]=='L')
				model_files.push_back( string(ent->d_name) );
		closedir(dir);
	}
}

vector< string > parse_filename( string x )
{
	vector< string > seq;
	string tmp = "";
	x.push_back( 0 );
	for (int a=0; x[a]; a++)
	{
		if ( ('a'<=x[a] && x[a]<='z') || ('A'<=x[a] && x[a]<='Z') )
			tmp.push_back( x[a] );
		else
		{
			seq.push_back( tmp );
			tmp = "";
			for (int b=a; x[b]; b++)
				if ('0'<=x[b] && x[b]<='9')
					tmp.push_back( x[b] );
				else
				{
					int num;
					sscanf_s( tmp.c_str(), "%d", &num );
					char str[10];
					sprintf_s( str, "%d", num );
					seq.push_back( string( str ) );
					tmp = "";
					for (int c=b+1; x[c]; c++)
						if (x[c]=='.' || x[c]==0)
						{
							seq.push_back( tmp );
							tmp = "";
							for (int d=c+1; x[d]; d++)
								tmp.push_back( x[d] );
							seq.push_back( tmp );
							break;
						}
						else tmp.push_back( x[c] );
					break;
				}
			break;
		}
	}
	return seq;
}

void load_model_file( string file )
{
	cerr << "loading model file " << file.c_str() << "\n";

	FILE * f = fopen( ( string("../data/problemsF/") + file ).c_str(), "rb" );
	if (!f)
	{
		cerr << "cannot open " << file.c_str() << ":(\n";
		return;
	}

	unsigned char xr;
	const size_t sz = fread_s( &xr, 1, 1, 1, f );
	int r = xr;
	vm.R = r;
	vm.filled = 0;
	int i=0, j=0, k=0;
	for (int a=0; a<((r*r*r+7)/8); a++)
	{
		unsigned char z;
		const size_t sz = fread_s( &z, 1, 1, 1, f );
		for (int b=0; b<8; b++)
		{
			vm.m[i][j][k] = ((z>>b)&1);
			if (vm.m[i][j][k]) vm.filled++;
			k++;
			if (k==r) { k=0; j++; }
			if (j==r) { j=0; i++; }
			if (i==r) break;
		}
	}

	fclose( f );

	cerr << "ok! R=" << r << "\n";

	cerr << "loading list of traces\n";

	vector< string > infseq = parse_filename( file );

	trace_files.clear();
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir("../data/tracesF/")) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
			if (ent->d_name[0]!='.')
			{
				vector< string > seq = parse_filename( string(ent->d_name) );

				if (seq[0]==infseq[0] && seq[1]==infseq[1] && seq[3]=="nbt.gz")
				{
					for (int a=0; a<(int)seq.size(); a++)
						cerr << seq[a].c_str() << " ";
					cerr << "\n";
					trace_files.push_back( make_pair( seq[2], string(ent->d_name) ) );
				}
			}
		closedir(dir);
	}
	cur_trace = "not selected";
	trace_cmd.clear();
	cerr << "ok! traces=" << trace_files.size() << "\n";
}

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
	while(1)
	{
		TraceCommand cmd = tr.read_next();
		if (cmd.tp == CT_UNDEFINED) break;
		trace_cmd.push_back( cmd );
		if (cmd.tp == CT_HALT) break;
	}

	cerr << "ok! commands=" << (int)trace_cmd.size() << "\n";
}

void nano_display_code()
{
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	ImGui::Begin( "Nanovisu~" );
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

	static bool need_refresh = true;
	if (ImGui::BeginCombo("Model", cur_model.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
	{
		if (need_refresh)
		{
			refresh_list_of_model_files();
			need_refresh = false;
		}
		for (int a = 0; a < (int)model_files.size(); a ++)
		{
			bool is_selected = (cur_model == model_files[a]);
			if (ImGui::Selectable(model_files[a].c_str(), is_selected))
			{
				trace_mode = false;
				ss.reset();
				cur_cmd = 0;
				cur_model = model_files[a];
				load_model_file( cur_model );
				need_refresh = true;
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
		}
		ImGui::EndCombo();
	}

	ImGui::Text( "Resolution: %d  Filled: %d\n", vm.R, vm.filled );

	if (ImGui::BeginCombo("Solver", cur_trace.c_str(), 0))
	{
		for (int a = 0; a < (int)trace_files.size(); a ++)
		{
			bool is_selected = (cur_trace == trace_files[a].first);
			if (ImGui::Selectable(trace_files[a].first.c_str(), is_selected))
			{
				trace_mode = false;
				ss.reset();
				cur_cmd = 0;
				cur_trace = trace_files[a].first;
				load_trace_file( trace_files[a].second );
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
		}
		ImGui::EndCombo();
	}

	//char str[100];
	ImGui::Text( "Trace Commands [%d/%d]", cur_cmd, (int)trace_cmd.size() );
	//if (ImGui::TreeNode( str ))
	{
		ImGui::BeginChild( "trace", ImVec2(0, ImGui::GetFrameHeightWithSpacing()*10 + 30), true );
		ImGuiListClipper clipper(trace_cmd.size());
		while (clipper.Step())
		{
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				char str[10];
				sprintf_s( str, "%d", i );
				ImGui::Text( str );
				ImGui::SameLine();
				float hue = (int)trace_cmd[i].tp*0.08f;
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hue, 0.6f, 0.6f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(hue, 0.7f, 0.7f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(hue, 0.8f, 0.8f));
				ImGui::Button( trace_cmd[i].cmd_to_string().c_str() );
				ImGui::PopStyleColor(3);
			}
		}
		ImGui::EndChild();
		//ImGui::TreePop();
	}

	if (trace_mode)
	{
		if (ImGui::Button( "Pause" ))
			trace_speed = 0;
		ImGui::SameLine();
		if (ImGui::Button( "Speed 1" ))
			trace_speed = 1;
		ImGui::SameLine();
		if (ImGui::Button( "Speed 5" ))
			trace_speed = 5;
		ImGui::SameLine();
		if (ImGui::Button( "Speed 10" ))
			trace_speed = 10;
		ImGui::SameLine();
		if (ImGui::Button( "Step" ))
		{
			trace_speed = 0;
			if (cur_cmd < (int)trace_cmd.size())
			{
				ss.perform_command( trace_cmd[cur_cmd] );
				cur_cmd++;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button( "Stop" ))
		{
			trace_mode = false;
			ss.reset();
			cur_cmd = 0;
		}
		ImGui::Text( "Speed: %d\n", trace_speed );
	}
	else
	{
		if (ImGui::Button( "Trace" ))
		{
			trace_mode = true;
			ss.reset();
			cur_cmd = 0;
			trace_speed = 1;
		}
		ImGui::SameLine();
		if (ImGui::Button( "Trace Step" ))
		{
			trace_mode = true;
			ss.reset();
			cur_cmd = 0;
			trace_speed = 0;
		}
	}

	ImGui::End();
}

void display_func()
{
	// Start the ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplFreeGLUT_NewFrame();

	//my_display_code();

	nano_display_code();

	// Rendering
    ImGui::Render();
    ImGuiIO& io = ImGui::GetIO();
    glViewport(200, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	GLfloat fog_color[4]= {0.f, 0.f, 0.f, 1.0f};
	glFogfv(GL_FOG_COLOR, fog_color);
	glFogf(GL_FOG_DENSITY, 0.01f);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogf(GL_FOG_START, 4.0f);
	glFogf(GL_FOG_END, 7.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, 1.*io.DisplaySize.x/io.DisplaySize.y, 0.1, 100.0);

	static double ang_alpha = 0., ang_beta = 0.;
	if (io.MouseDown[0] && !io.WantCaptureMouse)
	{
		ang_alpha += io.MouseDelta.x*0.005;
		ang_beta += io.MouseDelta.y*0.005;
		if (ang_beta > 0.49) ang_beta = 0.49;
		if (ang_beta < -0.49) ang_beta = -0.49;
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt( cos( ang_beta*pi ) * sin( ang_alpha*pi ) * 2., sin( ang_beta*pi )*2., cos( ang_beta*pi ) * cos( ang_alpha*pi ) * 2.,  /* eye is at */
    0.0, 0.0, 0.0,      /* center is at */
    0.0, 1.0, 0.0);      /* up is in direction */

    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (trace_mode)
	{
		draw_voxel_matrix( ss.ss_vm );
		double dd = 1. / ss.ss_vm.R;
		for (int a=0; a<kMaxBots; a++)
			if (ss.bots[a].active)
				draw_sphere(
					ss.bots[a].pos.x*dd + dd*0.5 - 0.5,
					ss.bots[a].pos.y*dd + dd*0.5 - 0.5,
					ss.bots[a].pos.z*dd + dd*0.5 - 0.5, dd * 0.4, 8 );

		for (int a=0; a<trace_speed; a++)
			if (cur_cmd < (int)trace_cmd.size())
			{
				ss.perform_command( trace_cmd[cur_cmd] );
				cur_cmd++;
			}
	}
	else draw_voxel_matrix( vm );
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glutSwapBuffers();
    glutPostRedisplay();
}

void idle_func()
{
	Sleep(20);
	glRotated(1.5, 1.0, 0.0, 0.0);
	glRotated(0.7, 0.0, 1.0, 0.0);
	glutPostRedisplay();
}

int main(int argc, char * argv[])
{
	sol();

	glutInit(&argc, argv);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);

	glutInitWindowSize(1280, 720);
	glutCreateWindow("WBM nanovisu~");

	glutDisplayFunc(display_func);

	// Setup ImGui binding
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    ImGui_ImplFreeGLUT_Init();
    ImGui_ImplFreeGLUT_InstallFuncs();
    ImGui_ImplOpenGL2_Init();

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();


	glutMainLoop();

	// Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplFreeGLUT_Shutdown();
    ImGui::DestroyContext();

	return 0;
}
