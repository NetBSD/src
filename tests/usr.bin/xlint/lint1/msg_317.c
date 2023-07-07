/*	$NetBSD: msg_317.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_317.c"

/* Test for message: __func__ is a C99 feature [317] */

/* lint1-flags: -sw -X 351 */

const char *
function(void)
{
	/* expect+1: warning: __func__ is a C99 feature [317] */
	return __func__;
}
