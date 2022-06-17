/*	$NetBSD: gcc_typeof_after_statement.c,v 1.3 2022/06/17 18:54:53 rillig Exp $	*/
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

/* Just to keep the .exp file. */
/* expect+1: warning: static function unused declared but not defined [290] */
static void unused(void);
