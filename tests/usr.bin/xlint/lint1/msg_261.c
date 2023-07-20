/*	$NetBSD: msg_261.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_261.c"

// Test for message: previous definition of '%s' [261]

/* lint1-extra-flags: -r -X 351 */

/* expect+2: previous definition of 'function' [261] */
void
function(void)
{
}

/* expect+2: error: redeclaration of 'function' with type 'function(void) returning int', expected 'function(void) returning void' [347] */
/* expect+1: warning: static function 'function' declared but not defined [290] */
static int function(void);
