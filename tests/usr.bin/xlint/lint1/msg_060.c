/*	$NetBSD: msg_060.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_060.c"

// Test for message: void must be sole parameter [60]

/* lint1-extra-flags: -X 351 */

void example_1(void);

/* expect+1: error: void must be sole parameter [60] */
void example_2(int, void);

/* expect+3: error: void must be sole parameter [60] */
/* expect+2: error: void must be sole parameter [60] */
/* expect+1: error: void must be sole parameter [60] */
void example_3(void, void, void);
