/*	$NetBSD: msg_021.c,v 1.2 2021/01/31 09:48:47 rillig Exp $	*/
# 3 "msg_021.c"

// Test for message: redeclaration of formal parameter %s [21]

/*ARGSUSED*/
void
old_style_with_duplicate_parameter(parameter, parameter) /* expect: 21 */
    int parameter;
{				/* expect: 32 */
}

void
old_style_with_duplicate_parameter_declaration(parameter)
    int parameter;
    int parameter;		/* expect: 237 */
{
}

void old_style_with_local_variable(parameter)
    int parameter;
{
	int parameter;		/* expect: 27 */
}

/*ARGSUSED*/
void
prototype_with_duplicate_parameter(int param, int param) /* expect: 237 */
{

}

void
prototype_with_local_variable(int parameter)
{
	int parameter;		/* expect: 27 */
}
