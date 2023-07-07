/*	$NetBSD: c11_atomic.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "c11_atomic.c"

/*
 * The keyword '_Atomic' was added in C11.  This test ensures that in C11
 * mode, '_Atomic' can be used as both type qualifier and type specifier.
 *
 * See also:
 *	C11 6.7.3 Type qualifiers
 *	C11 6.7.2.4 Atomic type specifiers
 *	msg_350.c
 */

/* lint1-extra-flags: -Ac11 -X 351 */

/* C11 6.7.3 "Type qualifiers" */
typedef _Atomic int atomic_int;

typedef _Atomic struct {
	int field;
} atomic_struct;

/* C11 6.7.2.4 "Atomic type specifiers" */
double *
atomic_ptr_cmpexch(_Atomic(double *)*ptr_var, _Atomic(double *) new_value)
{
	double *old = *ptr_var;
	*ptr_var = new_value;
	return old;
}
