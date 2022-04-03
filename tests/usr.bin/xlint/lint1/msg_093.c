/*	$NetBSD: msg_093.c,v 1.4 2022/04/03 09:34:45 rillig Exp $	*/
# 3 "msg_093.c"

// Test for message: dubious static function at block level: %s [93]

void
example(void)
{
	/* expect+1: warning: dubious static function at block level: nested [93] */
	static void nested(void);

	nested();
}
