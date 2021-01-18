/*	$NetBSD: msg_010.c,v 1.4 2021/01/18 17:43:44 rillig Exp $	*/
# 3 "msg_010.c"

// Test for message: duplicate '%s' [10]

inline inline void		/* expect: [10] */
double_inline(void)
{
}

const const int			/* expect: [10] */
double_const(void)
{
	return 0;
}

volatile volatile int		/* expect: [10] */
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

int
thread_local_parameter(_Thread_local int i) /* caught by the compiler */
{
	return i;
}
