/*	$NetBSD: msg_093.c,v 1.2 2021/01/07 00:38:46 rillig Exp $	*/
# 3 "msg_093.c"

// Test for message: dubious static function at block level: %s [93]

void
example(void)
{
	static void nested(void);

	nested();
}
