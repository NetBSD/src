/* libmain - flex run-time support library "main" function */

/* $Header: /cvsroot/src/usr.bin/lex/lib/Attic/libmain.c,v 1.1.1.1 1993/03/21 09:45:37 cgd Exp $ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];

    {
    return yylex();
    }
