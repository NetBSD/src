/*	$NetBSD: msg_383.c,v 1.1 2024/11/23 16:48:35 rillig Exp $	*/
# 3 "msg_383.c"

// Test for message: passing '%s' to argument %d discards '%s' [383]

/* lint1-extra-flags: -X 351 */

void sink_char(char *, const char *, volatile char *, const volatile char *);
void sink_int(int *, const int *, volatile int *, const volatile int *);

void
caller(const volatile char *cvcp, const volatile int *cvip, int (*fn)(void))
{
	/* expect+3: warning: passing 'pointer to const volatile char' to argument 1 discards 'const volatile' [383] */
	/* expect+2: warning: passing 'pointer to const volatile char' to argument 2 discards 'volatile' [383] */
	/* expect+1: warning: passing 'pointer to const volatile char' to argument 3 discards 'const' [383] */
	sink_char(cvcp, cvcp, cvcp, cvcp);
	/* expect+3: warning: passing 'pointer to const volatile int' to argument 1 discards 'const volatile' [383] */
	/* expect+2: warning: passing 'pointer to const volatile int' to argument 2 discards 'volatile' [383] */
	/* expect+1: warning: passing 'pointer to const volatile int' to argument 3 discards 'const' [383] */
	sink_int(cvip, cvip, cvip, cvip);
	/* expect+4: warning: converting 'pointer to function(void) returning int' to incompatible 'pointer to char' for argument 1 [153] */
	/* expect+3: warning: converting 'pointer to function(void) returning int' to incompatible 'pointer to const char' for argument 2 [153] */
	/* expect+2: warning: converting 'pointer to function(void) returning int' to incompatible 'pointer to volatile char' for argument 3 [153] */
	/* expect+1: warning: converting 'pointer to function(void) returning int' to incompatible 'pointer to const volatile char' for argument 4 [153] */
	sink_char(fn, fn, fn, fn);
}
