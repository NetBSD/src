/*	$NetBSD: msg_062.c,v 1.3 2022/02/27 20:02:44 rillig Exp $	*/
# 3 "msg_062.c"

// Test for message: function prototype parameters must have types [62]

outer() {
	/* expect+2: warning: function prototype parameters must have types [62] */
	/* expect+1: warning: dubious static function at block level: inner [93] */
	static int inner(a);
}
