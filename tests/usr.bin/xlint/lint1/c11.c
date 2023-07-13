/*	$NetBSD: c11.c,v 1.3 2023/07/13 20:30:21 rillig Exp $	*/
# 3 "c11.c"

/*
 * Test the language level C11, which adds _Generic expressions, _Noreturn
 * functions, anonymous struct/union members, and several more.
 */

/* lint1-flags: -Ac11 -w -X 236 -X 351 */

_Noreturn void exit(int);
void _Noreturn exit(int);

_Noreturn void
noreturn_before_type(void)
{
	exit(0);
}

void _Noreturn
noreturn_after_type(void)
{
	exit(0);
}

static _Noreturn void
noreturn_after_storage_class(void)
{
	exit(0);
}

_Noreturn static void
noreturn_before_storage_class(void)
{
	exit(0);
}

/* C11 6.7.4p5: A function specifier may appear more than once. */
_Noreturn _Noreturn _Noreturn void
three_times(void)
{
	exit(0);
}

// In C11 mode, 'thread_local' is not yet known, but '_Thread_local' is.
/* expect+2: error: old-style declaration; add 'int' [1] */
/* expect+1: error: syntax error 'int' [249] */
thread_local int thread_local_variable_c23;
_Thread_local int thread_local_variable_c11;

/* The '_Noreturn' must not appear after the declarator. */
void _Noreturn exit(int) _Noreturn;
/* expect-1: error: formal parameter #1 lacks name [59] */
/* expect-2: warning: empty declaration [2] */
/* expect+2: error: syntax error '' [249] */
/* expect+1: error: cannot recover from previous errors [224] */
