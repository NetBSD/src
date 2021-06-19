/*	$NetBSD: msg_249.c,v 1.3 2021/06/19 16:05:07 rillig Exp $	*/
# 3 "msg_249.c"

// Test for message: syntax error '%s' [249]

/*
 * Before func.c 1.110 from 2021-06-19, lint ran into this:
 * assertion "cstmt->c_kind == kind" failed in end_control_statement
 */
void
function(void)
{
	if (0)
		;
	);			/* expect: syntax error ')' */
}
