/*	$NetBSD: d_c99_decls_after_stmt.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "d_c99_decls_after_stmt.c"

/*
 * Before cgram.y 1.50 from 2011-10-04, lint complained about syntax errors
 * at the second 'int'.
 *
 * https://gnats.netbsd.org/45417
 */

/* lint1-extra-flags: -X 351 */

void
two_groups_of_decl_plus_stmt(void)
{
	int i = 0;
	i += 1;

	int j = 0;
	j += 1;
}

typedef int int_t;

int
second_decl_stmt_uses_a_typedef(void)
{
	int i = 0;
	i += 1;

	int_t j = 0;
	j += 1;

	return 0;
}

void
function_with_argument(int i)
{
	i += 1;

	int j = 0;
	j += 1;
}
