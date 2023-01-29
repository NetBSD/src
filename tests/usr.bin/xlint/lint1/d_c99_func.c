/*	$NetBSD: d_c99_func.c,v 1.4 2023/01/29 17:36:26 rillig Exp $	*/
# 3 "d_c99_func.c"

/* C99 __func__ */

const char *str;

const char *
function_name(void)
{
	/* FIXME: -14 for the array, not -8 for a pointer. */
	/* expect+1: error: negative array dimension (-8) [20] */
	typedef int reveal_size[-(int)sizeof(__func__)];
	return __func__;
}
