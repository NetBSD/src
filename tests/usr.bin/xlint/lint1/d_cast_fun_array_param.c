/*	$NetBSD: d_cast_fun_array_param.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_cast_fun_array_param.c"

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
