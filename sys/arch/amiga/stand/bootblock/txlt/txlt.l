	void munchit(char *);
%%
.\.l\ [._A-Za-z][A-Za-z0-9_.]*/\,	{printf("%c.l",yytext[0]);munchit(yytext+3);}
.\.w\ [._A-Za-z][A-Za-z0-9_.]*/\,	{printf("%c.w",yytext[0]);munchit(yytext+3);}
\..*\n					printf("%s", yytext);
pea[ 	][._A-Za-z][A-Za-z0-9_.]*$	{printf("pea");munchit(yytext+3);}
\ [._A-Za-z][A-Za-z0-9_.]*/\,		munchit(yytext);
.					putchar(*yytext);
%%
/*	$NetBSD: txlt.l,v 1.5 2011/06/20 20:25:47 he Exp $	*/

#include <string.h>

void
munchit(s)
	char *s;
{
	putchar(*s++);
	if ((!strncmp(s, "fp", 2) ||
	    !strncmp(s, "sp", 2) ||
	    ((*s == 'a')  || (*s == 'd')) && ((s[1]-'0')<=7)) &&
	    (s[2] == 0 || s[2] == ','))
		printf("%s", s);
	else
		printf("%%pc@(%s)",s);
}
