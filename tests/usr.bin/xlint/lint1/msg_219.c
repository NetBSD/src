/*	$NetBSD: msg_219.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_219.c"


/* Test for message: concatenated strings are illegal in traditional C [219] */

/* lint1-flags: -t -w */

char concat1[] = "one";
char concat2[] = "one" "two";			/* expect: 219 */
char concat3[] = "one" "two" "three";		/* expect: 219 */
char concat4[] = "one" "two" "three" "four";	/* expect: 219 */
