/*	$NetBSD: d_c99_func.c,v 1.5 2023/01/29 18:16:48 rillig Exp $	*/
# 3 "d_c99_func.c"

/* C99 __func__ */

const char *str;

const char *
function_name(void)
{
	/* expect+1: error: negative array dimension (-14) [20] */
	typedef int reveal_size[-(int)sizeof(__func__)];
	return __func__;
}
