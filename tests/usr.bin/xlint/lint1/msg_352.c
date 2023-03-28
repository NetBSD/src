/*	$NetBSD: msg_352.c,v 1.1 2023/03/28 20:04:52 rillig Exp $	*/
# 3 "msg_352.c"

// Test for message 352: nested 'extern' declaration of '%s' [352]

/*
 * C allows to declare external functions or objects inside function bodies,
 * which invites inconsistent types.
 *
 * Instead, any external functions or objects should be declared in headers.
 */

int
function(void)
{
	/* expect+1: warning: nested 'extern' declaration of 'external_func' [352] */
	extern int external_func(void);
	/* expect+1: warning: nested 'extern' declaration of 'external_var' [352] */
	extern int external_var;

	return external_func() + external_var;
}
