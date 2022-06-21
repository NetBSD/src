/*	$NetBSD: msg_093.c,v 1.5 2022/06/21 21:18:30 rillig Exp $	*/
# 3 "msg_093.c"

// Test for message: dubious static function '%s' at block level [93]

void
example(void)
{
	/* expect+1: warning: dubious static function 'nested' at block level [93] */
	static void nested(void);

	nested();
}
