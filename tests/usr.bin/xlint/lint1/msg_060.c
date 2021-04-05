/*	$NetBSD: msg_060.c,v 1.4 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_060.c"

// Test for message: void must be sole parameter [60]

void example_1(void);
void example_2(int, void);		/* expect: 60 */
void example_3(void, void, void);	/* expect: 60 *//* expect: 60 *//* expect: 60 */
