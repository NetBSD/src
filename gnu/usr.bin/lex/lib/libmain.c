/* libmain - flex run-time support library "main" function */

#ifndef lint
static char rcsid[] = "$Id: libmain.c,v 1.3 1993/08/02 17:46:32 mycroft Exp $";
#endif /* not lint */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];

    {
    return yylex();
    }
