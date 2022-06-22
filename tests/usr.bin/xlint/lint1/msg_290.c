/*	$NetBSD: msg_290.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_290.c"

// Test for message: static function '%s' declared but not defined [290]

/* expect+1: warning: static function 'only_declared' declared but not defined [290] */
static void only_declared(void);
static void declared_and_called(void);

void
use_function(void)
{
	/* expect+1: error: static function 'declared_and_called' called but not defined [225] */
	declared_and_called();
}
