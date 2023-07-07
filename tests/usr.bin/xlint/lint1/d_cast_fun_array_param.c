/*	$NetBSD: d_cast_fun_array_param.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "d_cast_fun_array_param.c"

/* lint1-extra-flags: -X 351 */

static void
f(void *b[4])
{
	(void)&b;
}

void *
foo(void *fn)
{
	return fn == 0 ? f : (void (*)(void *[4]))fn;
}
