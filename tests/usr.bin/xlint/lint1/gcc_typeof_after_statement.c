/*	$NetBSD: gcc_typeof_after_statement.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "gcc_typeof_after_statement.c"

/*
 * Before cgram.y 1.226 from 2021-05-03, lint could not parse typeof(...) if
 * there was a statement before it.
 */

/* lint1-extra-flags: -X 351 */

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
