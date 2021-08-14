/*	$NetBSD: msg_164.c,v 1.4 2021/08/14 12:46:24 rillig Exp $	*/
# 3 "msg_164.c"

// Test for message: assignment of negative constant to unsigned type [164]

void
example(void)
{
	/* expect+1: warning: initialization of unsigned with negative constant [221] */
	unsigned char uch = -3;

	uch = -5;			/* expect: 164 */
	uch += -7;			/* expect: 222 */
	uch *= -1;			/* expect: 222 */
}
