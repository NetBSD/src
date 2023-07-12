/*	$NetBSD: c11.c,v 1.1 2023/07/12 18:13:39 rillig Exp $	*/
# 3 "c11.c"

/*
 * Test the language level C11, which adds _Generic expressions, _Noreturn
 * functions, anonymous struct/union members, and several more.
 */

/* lint1-flags: -Ac11 -w -X 236 -X 351 */

_Noreturn void exit(int);
void _Noreturn exit(int);

/* XXX: Syntactically invalid, yet lint accepts it. */
void _Noreturn exit(int) _Noreturn;

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
