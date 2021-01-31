/*	$NetBSD: msg_093.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_093.c"

// Test for message: dubious static function at block level: %s [93]

void
example(void)
{
	static void nested(void);	/* expect: 93 */

	nested();
}
