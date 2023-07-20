/*	$NetBSD: msg_290.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_290.c"

// Test for message: static function '%s' declared but not defined [290]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: static function 'only_declared' declared but not defined [290] */
static void only_declared(void);
static void declared_and_called(void);

void
use_function(void)
{
	/* expect+1: error: static function 'declared_and_called' called but not defined [225] */
	declared_and_called();
}
