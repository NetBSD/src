/*	$NetBSD: msg_021.c,v 1.8 2025/01/03 03:14:47 rillig Exp $	*/
# 3 "msg_021.c"

// Test for message: redeclaration of formal parameter '%s' [21]

/* See also message 237, which has the same text. */

/* lint1-extra-flags: -X 351 */

/*ARGSUSED*/
void
/* expect+2: error: redeclaration of formal parameter 'parameter' [21] */
/* expect+1: warning: function definition for 'old_style_with_duplicate_parameter' with identifier list is obsolete in C23 [384] */
old_style_with_duplicate_parameter(parameter, parameter)
    int parameter;
{
	/* expect-1: warning: type of parameter 'parameter' defaults to 'int' [32] */
}

void
/* expect+1: warning: function definition for 'old_style_with_duplicate_parameter_declaration' with identifier list is obsolete in C23 [384] */
old_style_with_duplicate_parameter_declaration(parameter)
    int parameter;
    /* expect+1: error: redeclaration of formal parameter 'parameter' [237] */
    int parameter;
{
}

/* expect+1: warning: function definition for 'old_style_with_local_variable' with identifier list is obsolete in C23 [384] */
void old_style_with_local_variable(parameter)
    int parameter;
{
	/* expect+1: error: redeclaration of 'parameter' [27] */
	int parameter;
}

/*ARGSUSED*/
void
/* expect+1: error: redeclaration of formal parameter 'param' [237] */
prototype_with_duplicate_parameter(int param, int param)
{

}

void
prototype_with_local_variable(int parameter)
{
	/* expect+1: error: redeclaration of 'parameter' [27] */
	int parameter;
}
