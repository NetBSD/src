/*	$NetBSD: msg_060.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_060.c"

// Test for message: void must be sole parameter [60]

void example_1(void);
void example_2(int, void);		/* expect: 60 */
void example_3(void, void, void);	/* expect: 60, 60, 60 */
