/*	$NetBSD: msg_317.c,v 1.4 2022/02/27 12:00:27 rillig Exp $	*/
# 3 "msg_317.c"

/* Test for message: __func__ is a C99 feature [317] */

/* lint1-flags: -sw */

const char *
function(void)
{
	/* expect+1: warning: __func__ is a C99 feature [317] */
	return __func__;
}
