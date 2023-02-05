/*	$NetBSD: gcc_typeof_after_statement.c,v 1.5 2023/02/05 10:57:48 rillig Exp $	*/
# 3 "gcc_typeof_after_statement.c"

/*
 * Before cgram.y 1.226 from 2021-05-03, lint could not parse typeof(...) if
 * there was a statement before it.
 */

void *
example(void **ptr)
{
	return ({
		if (*ptr != (void *)0)
			ptr++;
		__typeof__(*ptr) ret = *ptr;
		ret;
	});
}
