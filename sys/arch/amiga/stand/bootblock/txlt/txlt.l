	void munchit(char *);
%%
\..*\n					printf("%s", yytext);
pea[ 	][_A-Za-z][A-Za-z0-9_]*$	{printf("pea");munchit(yytext+3);}
\ [_A-Za-z][A-Za-z0-9_]*/\,		munchit(yytext);
.					putchar(*yytext);
%%
/*	$NetBSD: txlt.l,v 1.2.14.1 2001/03/12 13:27:13 bouyer Exp $	*/

void
munchit(s)
	char *s;
{
	putchar(*s++);
	if (!strncmp(s, "fp", 2) ||
	    !strncmp(s, "sp", 2) ||
	    ((*s == 'a')  || (*s == 'd')) && ((s[1]-'0')<=7))
		printf("%s", s);
	else
		printf("%%pc@(%s)",s);
}
