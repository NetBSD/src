/*	$NetBSD: msg_310.c,v 1.6 2023/08/02 18:51:25 rillig Exp $	*/
# 3 "msg_310.c"

// Test for message: symbol renaming can't be used on function parameters [310]

/* lint1-extra-flags: -X 351 */

/* expect+2: warning: parameter 'callback' unused in function 'function' [231] */
void
function(int (*callback)(void) __symbolrename(parameter))
{
}

/* expect+1: error: syntax error ':' [249] */
TODO: "Add example code that triggers the above message."
TODO: "Add example code that almost triggers the above message."
