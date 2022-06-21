/*	$NetBSD: msg_062.c,v 1.5 2022/06/21 21:18:30 rillig Exp $	*/
# 3 "msg_062.c"

// Test for message: function prototype parameters must have types [62]

/* expect+1: error: old style declaration; add 'int' [1] */
outer() {
	/* expect+2: warning: function prototype parameters must have types [62] */
	/* expect+1: warning: dubious static function 'inner' at block level [93] */
	static int inner(a);
}
