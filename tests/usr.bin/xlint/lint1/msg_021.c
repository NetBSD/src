/*	$NetBSD: msg_021.c,v 1.3 2022/06/11 12:24:00 rillig Exp $	*/
# 3 "msg_021.c"

// Test for message: redeclaration of formal parameter '%s' [21]

/* See also message 237, which has the same text. */

/*ARGSUSED*/
void
/* expect+1: error: redeclaration of formal parameter 'parameter' [21] */
old_style_with_duplicate_parameter(parameter, parameter)
    int parameter;
{
	/* expect-1: warning: argument type defaults to 'int': parameter [32] */
}

void
old_style_with_duplicate_parameter_declaration(parameter)
    int parameter;
    /* expect+1: error: redeclaration of formal parameter 'parameter' [237] */
    int parameter;
{
}

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
