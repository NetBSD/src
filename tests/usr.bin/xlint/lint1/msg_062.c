/*	$NetBSD: msg_062.c,v 1.8 2024/12/01 18:37:54 rillig Exp $	*/
# 3 "msg_062.c"

// Test for message: function prototype parameters must have types [62]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: old-style declaration; add 'int' [1] */
outer() {
	/* expect+3: warning: function definition with identifier list is obsolete in C23 [384] */
	/* expect+2: warning: function prototype parameters must have types [62] */
	/* expect+1: warning: dubious static function 'inner' at block level [93] */
	static int inner(a);
}
