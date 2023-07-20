/*	$NetBSD: msg_010.c,v 1.7 2023/07/13 19:59:08 rillig Exp $	*/
# 3 "msg_010.c"

// Test for message: duplicate '%s' [10]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: duplicate 'inline' [10] */
inline inline void
double_inline(void)
{
}

/* expect+1: warning: duplicate 'const' [10] */
const const int
double_const(void)
{
	return 0;
}

/* expect+1: warning: duplicate 'volatile' [10] */
volatile volatile int
double_volatile(void)
{
	return 0;
}

int
restrict_pointer(const int *restrict p)
{
	return *p;
}

_Thread_local int thread_local_int;
_Thread_local int *pointer_to_thread_local;

/* expect+2: error: only 'register' is valid as storage class in parameter [9] */
int
thread_local_parameter(_Thread_local int i)
{
	return i;
}
