/*	$NetBSD: gcc_typeof_after_statement.c,v 1.1 2021/04/22 22:43:26 rillig Exp $	*/
# 3 "gcc_typeof_after_statement.c"

/*
 * As of 2021-04-23, lint cannot parse typeof(...) if there is a statement
 * before it.
 */

void *
example(void **ptr)
{
	return ({
		if (*ptr != (void *)0)
			ptr++;

		/* FIXME: This is a legitimate use case. */
		/* expect+1: syntax error '__typeof__' [249] */
		__typeof__(*ptr) ret = *ptr;
		/* expect+1: 'ret' undefined [99] */
		ret;
		/* expect+1: illegal combination of pointer (pointer to void) and integer (int) [183] */
	});
}
