/*	$NetBSD: msg_086.c,v 1.7.2.1 2025/08/02 05:58:16 perseant Exp $	*/
# 3 "msg_086.c"

// Test for message: automatic '%s' hides external declaration with type '%s' [86]

/* lint1-flags: -S -g -h -w -X 351 */

extern double variable;
void parameter(double);
void err(int, const char *, ...);

int sink;

void
/* XXX: the function parameter does not trigger the warning. */
local_(int parameter)
{
	/* expect+1: warning: automatic 'variable' hides external declaration with type 'double' [86] */
	int variable = 3;
	/* expect+1: warning: automatic 'err' hides external declaration with type 'function(int, pointer to const char, ...) returning void' [86] */
	int err = 5;

	sink = variable;
	sink = parameter;
	sink = err;
}
