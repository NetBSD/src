/*	$NetBSD: msg_352.c,v 1.2 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_352.c"

// Test for message 352: nested 'extern' declaration of '%s' [352]

/*
 * C allows to declare external functions or objects inside function bodies,
 * which invites inconsistent types.
 *
 * Instead, any external functions or objects should be declared in headers.
 */

/* lint1-extra-flags: -X 351 */

int
function(void)
{
	/* expect+1: warning: nested 'extern' declaration of 'external_func' [352] */
	extern int external_func(void);
	/* expect+1: warning: nested 'extern' declaration of 'external_var' [352] */
	extern int external_var;

	return external_func() + external_var;
}
