/*	$NetBSD: msg_092.c,v 1.4 2022/06/21 21:18:30 rillig Exp $	*/
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

	/* expect+2: warning: dubious static function 'random' at block level [93] */
	/* expect+1: warning: inconsistent redeclaration of static 'random' [92] */
	static double random(void);
}
