/*
 *	This simple classical example of recursion is useful for
 *	testing stack backtraces and such.
 */

#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[], char **envp)
/*int argc;
char *argv[], **envp;*/
{
    int factorial (int);
#ifdef FAKEARGV
    printf ("%d\n", factorial (1));
#else    
    if (argc != 2) {
	printf ("usage:  factorial <number>\n");
	return 1;
    } else {
	printf ("%d\n", factorial (atoi (argv[1])));
    }
#endif
    return 0;
}

int factorial (int value)
/*int value;*/
{
    int  local_var;

    if (value > 1) {
	value *= factorial (value - 1);
    }
    local_var = value;
    return (value);
}
