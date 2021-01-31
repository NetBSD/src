/*	$NetBSD: msg_164.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_164.c"

// Test for message: assignment of negative constant to unsigned type [164]

void
example(void)
{
	unsigned char uch = -3;		/* expect: 164 */

	uch = -5;			/* expect: 164 */
	uch += -7;			/* expect: 222 */
	uch *= -1;			/* expect: 222 */
}
