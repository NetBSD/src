/*	$NetBSD: msg_019.c,v 1.5 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_019.c"

// Test for message: void type for '%s' [19]

/* expect+1: error: void type for 'global_variable' [19] */
void global_variable;

/* expect+2: error: void type for 'unit_variable' [19] */
/* expect+1: warning: static variable 'unit_variable' unused [226] */
static void unit_variable;

/* expect+3: warning: argument 'parameter' unused in function 'function' [231] */
/* expect+2: error: void parameter cannot have name: parameter [61] */
void
function(void parameter)
{
	/* expect+1: error: void type for 'local_variable' [19] */
	void local_variable;
}
