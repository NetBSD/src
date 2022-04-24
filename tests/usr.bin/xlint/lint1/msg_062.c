/*	$NetBSD: msg_062.c,v 1.4 2022/04/24 20:08:23 rillig Exp $	*/
# 3 "msg_062.c"

// Test for message: function prototype parameters must have types [62]

/* expect+1: error: old style declaration; add 'int' [1] */
outer() {
	/* expect+2: warning: function prototype parameters must have types [62] */
	/* expect+1: warning: dubious static function at block level: inner [93] */
	static int inner(a);
}
