/*	$NetBSD: msg_333.c,v 1.6 2023/05/11 08:01:36 rillig Exp $	*/
# 3 "msg_333.c"

// Test for message: controlling expression must be bool, not '%s' [333]
//
// See d_c99_bool_strict.c for many more examples.

/* lint1-extra-flags: -T */

typedef _Bool bool;

static enum tagged_color {
	tagged_red,
} e1;
typedef enum {
	typedef_red,
} typedef_color;
static typedef_color e2;

const char *
example(bool b, int i, const char *p)
{

	if (b)
		return "bool";

	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (i)
		return "int";

	/* expect+1: error: controlling expression must be bool, not 'enum tagged_color' [333] */
	if (e1)
		return "tagged enum";

	/* expect+1: error: controlling expression must be bool, not 'enum typedef typedef_color' [333] */
	if (e2)
		return "typedef enum";

	/* expect+1: error: controlling expression must be bool, not 'pointer' [333] */
	if (p)
		return "pointer";

	if (__lint_false) {
		/* expect+1: warning: statement not reached [193] */
		return "bool constant";
	}

	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (0) {
		/* expect+1: warning: statement not reached [193] */
		return "integer constant";
	}

	return p + i;
}
