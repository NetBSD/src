/*	$NetBSD: msg_092.c,v 1.5 2023/07/07 06:03:31 rillig Exp $	*/
# 3 "msg_092.c"

// Test for message: inconsistent redeclaration of static '%s' [92]

static int
random(void)
{
	return 4;
}

void
use_random(void)
{
	random();

	/* expect+3: warning: 'random' unused in function 'use_random' [192] */
	/* expect+2: warning: dubious static function 'random' at block level [93] */
	/* expect+1: warning: inconsistent redeclaration of static 'random' [92] */
	static double random(void);
}
