/*	$NetBSD: msg_316.c,v 1.3 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_316.c"

// Test for message: __FUNCTION__/__PRETTY_FUNCTION__ is a GCC extension [316]

/* lint1-flags: -Sw */

void println(const char *);

void debug(void)
{
	/* expect+1: warning: __FUNCTION__/__PRETTY_FUNCTION__ is a GCC extension [316] */
	println(__FUNCTION__);
	/* expect+1: warning: __FUNCTION__/__PRETTY_FUNCTION__ is a GCC extension [316] */
	println(__PRETTY_FUNCTION__);
}
