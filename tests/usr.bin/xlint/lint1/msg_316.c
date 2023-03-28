/*	$NetBSD: msg_316.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_316.c"

// Test for message: __FUNCTION__/__PRETTY_FUNCTION__ is a GCC extension [316]

/* lint1-flags: -Sw -X 351 */

void println(const char *);

void debug(void)
{
	/* expect+1: warning: __FUNCTION__/__PRETTY_FUNCTION__ is a GCC extension [316] */
	println(__FUNCTION__);
	/* expect+1: warning: __FUNCTION__/__PRETTY_FUNCTION__ is a GCC extension [316] */
	println(__PRETTY_FUNCTION__);
}
