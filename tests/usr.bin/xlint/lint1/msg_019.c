/*	$NetBSD: msg_019.c,v 1.9 2024/01/28 08:54:27 rillig Exp $	*/
# 3 "msg_019.c"

// Test for message: void type for '%s' [19]

/* lint1-extra-flags: -X 351 */

/* expect+1: error: void type for 'global_variable' [19] */
void global_variable;

/* expect+2: error: void type for 'unit_variable' [19] */
/* expect+1: warning: static variable 'unit_variable' unused [226] */
static void unit_variable;

/* expect+3: error: void parameter 'parameter' cannot have name [61] */
/* expect+2: warning: parameter 'parameter' unused in function 'function' [231] */
void
function(void parameter)
{
	/* expect+1: error: void type for 'local_variable' [19] */
	void local_variable;
}
