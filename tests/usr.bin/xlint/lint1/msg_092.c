/*	$NetBSD: msg_092.c,v 1.3 2022/04/03 09:34:45 rillig Exp $	*/
# 3 "msg_092.c"

// Test for message: inconsistent redeclaration of static: %s [92]

static int
random(void)
{
	return 4;
}

void
use_random(void)
{
	random();

	/* expect+2: warning: dubious static function at block level: random [93] */
	/* expect+1: warning: inconsistent redeclaration of static: random [92] */
	static double random(void);
}
