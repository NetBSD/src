/*	$NetBSD: d_c99_func.c,v 1.7 2024/01/07 12:43:16 rillig Exp $	*/
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


// Since tree.c 1.504 from 2023-01-29 and before tree.c 1.591 from 2024-01-07,
// lint crashed because there was no "current function", even though the "block
// level" was not 0.
/* expect+1: error: '__func__' undefined [99] */
typedef int f(int[sizeof(__func__)]);
