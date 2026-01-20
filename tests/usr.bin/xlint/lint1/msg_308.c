/*	$NetBSD: msg_308.c,v 1.10 2026/01/20 23:33:05 rillig Exp $	*/
# 3 "msg_308.c"

// Test for message: invalid type for _Complex [308]

/* lint1-extra-flags: -X 351 */

float _Complex float_complex;
double _Complex double_complex;
long double _Complex long_double_complex;

/* expect+1: error: invalid type for _Complex [308] */
_Complex plain_complex;

/* expect+2: error: invalid type for _Complex [308] */
/* expect+1: error: invalid type combination [4] */
int _Complex int_complex;

void *ptr;

void
reveal_types(void)
{
	/* expect+1: ... 'float _Complex' [171] */
	ptr = (_Complex float)0.0;
	/* expect+1: ... 'float _Complex' [171] */
	ptr = (float _Complex)0.0;
	/* expect+1: ... 'double _Complex' [171] */
	ptr = (_Complex double)0.0;
	/* expect+1: ... 'double _Complex' [171] */
	ptr = (double _Complex)0.0;
	/* expect+1: ... 'long double _Complex' [171] */
	ptr = (_Complex double long)0.0;
	/* expect+1: ... 'long double _Complex' [171] */
	ptr = (_Complex long double)0.0;
	/* expect+1: ... 'long double _Complex' [171] */
	ptr = (double _Complex long)0.0;
	/* expect+1: ... 'long double _Complex' [171] */
	ptr = (double long _Complex)0.0;
	/* expect+1: ... 'long double _Complex' [171] */
	ptr = (long _Complex double)0.0;
	/* expect+1: ... 'long double _Complex' [171] */
	ptr = (long double _Complex)0.0;
}
