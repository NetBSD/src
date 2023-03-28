/*	$NetBSD: d_c99_func.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_func.c"

/* C99 __func__ */

/* lint1-extra-flags: -X 351 */

const char *str;

const char *
function_name(void)
{
	/* expect+1: error: negative array dimension (-14) [20] */
	typedef int reveal_size[-(int)sizeof(__func__)];
	return __func__;
}
