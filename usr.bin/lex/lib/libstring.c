/* libstring - flex run-time string routines */

/* $Header: /cvsroot/src/usr.bin/lex/lib/Attic/libstring.c,v 1.1 1993/12/06 19:26:05 jtc Exp $ */

/* These routines exist only because it's a pain to portably include
 * <string.h>/<strings.h>, and the generated scanner needs access to
 * strcpy() to support yytext.
 */

extern int yy_strcmp();
extern void yy_strcpy();
extern int yy_strlen();


int yy_strcmp( s1, s2 )
const char *s1;
const char *s2;
	{
	while ( *s1 && *s2 )
		{
		unsigned char uc1 = (unsigned char) *s1;
		unsigned char uc2 = (unsigned char) *s2;

		if ( uc1 > uc2 )
			return 1;

		else if ( uc1 < uc2 )
			return -1;

		++s1;
		++s2;
		}

	if ( *s1 )
		/* s1 is longer than s2, so s1 > s2 */
		return 1;

	else if ( *s2 )
		return -1;

	else
		return 0;
	}

void yy_strcpy( s1, s2 )
char *s1;
const char *s2;
	{
	while ( (*(s1++) = *(s2++)) )
		;
	}

int yy_strlen( s )
const char* s;
	{
	int len = 0;

	while ( s[len] )
		++len;

	return len;
	}
