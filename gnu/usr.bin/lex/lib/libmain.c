/* libmain - flex run-time support library "main" function */

/* $Header: /cvsroot/src/gnu/usr.bin/lex/lib/Attic/libmain.c,v 1.2 1993/05/04 07:45:30 cgd Exp $ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];

    {
    return yylex();
    }
