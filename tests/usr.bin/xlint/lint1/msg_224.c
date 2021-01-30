/*	$NetBSD: msg_224.c,v 1.2 2021/01/30 17:02:58 rillig Exp $	*/
# 3 "msg_224.c"

// Test for message: cannot recover from previous errors [224]

void example1(void) { "syntax" error; }		/* expect: 249 */
void example2(void) { "syntax" error; }		/* expect: 249 */
void example3(void) { "syntax" error; }		/* expect: 249 */
void example4(void) { "syntax" error; }		/* expect: 249 */
void example5(void) { "syntax" error; }		/* expect: 249, 224 */
void example6(void) { "syntax" error; }
