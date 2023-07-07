/*	$NetBSD: msg_093.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_093.c"

// Test for message: dubious static function '%s' at block level [93]

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	/* expect+1: warning: dubious static function 'nested' at block level [93] */
	static void nested(void);

	nested();
}
