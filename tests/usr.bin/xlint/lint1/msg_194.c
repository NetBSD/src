/*	$NetBSD: msg_194.c,v 1.2 2021/01/30 17:56:29 rillig Exp $	*/
# 3 "msg_194.c"

// Test for message: label %s redefined [194]

void example(void)
{
	int i = 0;
label:				/* expect: 232 */
	i = 1;
label:				/* expect: 194 */
	i = 2;
}
