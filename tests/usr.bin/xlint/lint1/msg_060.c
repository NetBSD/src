/*	$NetBSD: msg_060.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_060.c"

// Test for message: void must be sole parameter [60]

void example_1(void);
void example_2(int, void);
void example_3(void, void, void);
