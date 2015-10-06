#include <cstdio>
#include <cstdlib>
#include <cstring>


bool initial = true;
FILE * debug_out;

void assert(bool condition, char * message)
{

	if(!condition)
	{
		if(initial)
		{
			debug_out = fopen("debug.txt","w");
			initial = false;
		}
		fprintf(debug_out, "%s\n",message);
		fflush(debug_out);
	}
}
void assert(bool condition, char * message, int val)
{
	throw;
}