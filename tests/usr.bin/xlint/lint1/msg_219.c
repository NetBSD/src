/*	$NetBSD: msg_219.c,v 1.6.2.1 2025/08/02 05:58:17 perseant Exp $	*/
# 3 "msg_219.c"


/* Test for message: concatenated strings require C90 or later [219] */

/* lint1-flags: -t -w -X 351 */

char concat1[] = "one";
/* expect+1: warning: concatenated strings require C90 or later [219] */
char concat2[] = "one" "two";
/* expect+2: warning: concatenated strings require C90 or later [219] */
/* expect+1: warning: concatenated strings require C90 or later [219] */
char concat3[] = "one" "two" "three";
/* expect+3: warning: concatenated strings require C90 or later [219] */
/* expect+2: warning: concatenated strings require C90 or later [219] */
/* expect+1: warning: concatenated strings require C90 or later [219] */
char concat4[] = "one" "two" "three" "four";

char concat4lines[] =
	"one"
	/* expect+1: warning: concatenated strings require C90 or later [219] */
	"two"
	/* expect+1: warning: concatenated strings require C90 or later [219] */
	"three"
	/* expect+1: warning: concatenated strings require C90 or later [219] */
	"four";
