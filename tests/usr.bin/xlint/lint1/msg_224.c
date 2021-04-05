/*	$NetBSD: msg_224.c,v 1.3 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_224.c"

// Test for message: cannot recover from previous errors [224]

void example1(void) { "syntax" error; }		/* expect: 249 */
void example2(void) { "syntax" error; }		/* expect: 249 */
void example3(void) { "syntax" error; }		/* expect: 249 */
void example4(void) { "syntax" error; }		/* expect: 249 */
void example5(void) { "syntax" error; }		/* expect: 249 *//* expect: 224 */
void example6(void) { "syntax" error; }
