	void munchit(char *);
%%
.\.l\ [._A-Za-z][A-Za-z0-9_.]*/\,	{printf("%c.l",yytext[0]);munchit(yytext+3);}
.\.w\ [._A-Za-z][A-Za-z0-9_.]*/\,	{printf("%c.w",yytext[0]);munchit(yytext+3);}
\..*\n					printf("%s", yytext);
pea[ 	][._A-Za-z][A-Za-z0-9_.]*$	{printf("pea");munchit(yytext+3);}
\ [._A-Za-z][A-Za-z0-9_.]*/\,		munchit(yytext);
.					putchar(*yytext);
%%
/*	$NetBSD: txlt.l,v 1.5.12.1 2017/12/03 11:35:49 jdolecek Exp $	*/

#include <string.h>

void
munchit(char *s) {

	putchar(*s++);
	if ((!strncmp(s, "fp", 2) ||
	    !strncmp(s, "sp", 2) ||
	    ((*s == 'a')  || (*s == 'd')) && ((s[1]-'0')<=7)) &&
	    (s[2] == 0 || s[2] == ','))
		printf("%s", s);
	else
		printf("%%pc@(%s)",s);
}

int
main(void) {
	while (yylex() != 0)
		;

	return 0;
}
