/*	$NetBSD: msg_267.c,v 1.5 2022/08/19 19:13:04 rillig Exp $	*/
# 3 "msg_267.c"

// Test for message: shift equal to size of object [267]

int
shr32(unsigned int x)
{
	/* expect+1: warning: shift equal to size of object [267] */
	return x >> 32;
}

int
shl32(unsigned int x)
{
	/* expect+1: warning: shift equal to size of object [267] */
	return x << 32;
}

/*
 * As of 2022-08-19, lint ignores the GCC-specific 'mode' attribute, treating
 * the tetra-int as a plain single-int, thus having width 32.
 *
 * https://gcc.gnu.org/onlinedocs/gccint/Machine-Modes.html
 */
unsigned
function(unsigned __attribute__((mode(TI))) arg)
{
	/* XXX: The 'size' usually means the size in bytes, not in bits. */
	/* expect+1: warning: shift equal to size of object [267] */
	return (arg >> 32) & 3;
}
