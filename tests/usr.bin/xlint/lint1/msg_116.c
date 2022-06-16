/*	$NetBSD: msg_116.c,v 1.5 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_116.c"

// Test for message: illegal pointer subtraction [116]

/*
 * Subtracting an int pointer from a double pointer does not make sense.
 * The result cannot be reasonably defined since it is "the difference of
 * the subscripts of the two array elements" (C99 6.5.5p9), and these two
 * pointers cannot point to the same array.
 */
_Bool
example(int *a, double *b)
{
	/* expect+1: error: illegal pointer subtraction [116] */
	return a - b > 0;
}

/*
 * Even though signed char and unsigned char have the same size,
 * their pointer types are still considered incompatible.
 *
 * C99 6.5.5p9
 */
_Bool
subtract_character_pointers(signed char *scp, unsigned char *ucp)
{
	/* expect+1: error: illegal pointer subtraction [116] */
	return scp - ucp > 0;
}

_Bool
subtract_const_pointer(const char *ccp, char *cp)
{
	return ccp - cp > 0;
}
