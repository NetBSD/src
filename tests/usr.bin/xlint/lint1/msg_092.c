/*	$NetBSD: msg_092.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_092.c"

// Test for message: inconsistent redeclaration of static '%s' [92]

/* lint1-extra-flags: -X 351 */

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
