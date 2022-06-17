/*	$NetBSD: msg_310.c,v 1.3 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_310.c"

// Test for message: symbol renaming can't be used on function arguments [310]

/* expect+2: warning: argument 'callback' unused in function 'function' [231] */
void
function(int (*callback)(void) __symbolrename(argument))
{
}

/* expect+1: error: syntax error ':' [249] */
TODO: "Add example code that triggers the above message."
TODO: "Add example code that almost triggers the above message."
