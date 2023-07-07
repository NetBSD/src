/*	$NetBSD: msg_350.c,v 1.3 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_350.c"

// Test for message 350: '_Atomic' requires C11 or later [350]

/*
 * The keyword '_Atomic' was added in C11.  This test ensures that in C99
 * mode, the parser recognizes the keyword but flags it.
 *
 * See also:
 *	c11_atomic.c
 */

/* lint1-extra-flags: -X 351 */

/* expect+1: error: '_Atomic' requires C11 or later [350] */
typedef _Atomic int atomic_int;

/* expect+1: error: '_Atomic' requires C11 or later [350] */
typedef _Atomic struct {
	int field;
} atomic_struct;

/* expect+3: error: '_Atomic' requires C11 or later [350] */
/* expect+2: error: '_Atomic' requires C11 or later [350] */
double *
atomic_ptr_cmpexch(_Atomic(double *)*ptr_var, _Atomic(double *)new_value)
{
	double *old = *ptr_var;
	*ptr_var = new_value;
	return old;
}
