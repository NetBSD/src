/*	$NetBSD: d_c99_decls_after_stmt.c,v 1.4 2022/02/12 01:23:44 rillig Exp $	*/
# 3 "d_c99_decls_after_stmt.c"

/*
 * Before cgram.y 1.50 from 2011-10-04, lint complained about syntax errors
 * at the second 'int'.
 *
 * https://gnats.netbsd.org/45417
 */

void
sample(void)
{
	int i = 0;
	i += 1;

	int j = 0;
	j += 1;
}
