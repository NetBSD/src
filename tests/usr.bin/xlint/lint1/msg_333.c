/*	$NetBSD: msg_333.c,v 1.4 2021/07/04 07:09:39 rillig Exp $	*/
# 3 "msg_333.c"

// Test for message: controlling expression must be bool, not '%s' [333]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

const char *
example(bool b, int i, const char *p)
{

	if (b)
		return "bool";

	/* expect+1: must be bool, not 'int' [333] */
	if (i)
		return "int";

	/* expect+1: must be bool, not 'pointer' [333] */
	if (p)
		return "pointer";

	if (__lint_false) {
		/* expect+1: warning: statement not reached [193] */
		return "bool constant";
	}

	/* expect+1: controlling expression must be bool, not 'int' [333] */
	if (0) {
		/* expect+1: warning: statement not reached [193] */
		return "integer constant";
	}

	return p + i;
}
