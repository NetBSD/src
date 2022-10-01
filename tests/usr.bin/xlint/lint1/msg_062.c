/*	$NetBSD: msg_062.c,v 1.6 2022/10/01 09:42:40 rillig Exp $	*/
# 3 "msg_062.c"

// Test for message: function prototype parameters must have types [62]

/* expect+1: error: old-style declaration; add 'int' [1] */
outer() {
	/* expect+2: warning: function prototype parameters must have types [62] */
	/* expect+1: warning: dubious static function 'inner' at block level [93] */
	static int inner(a);
}
