/* $NetBSD: opt_lc.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the option '-lc', which specifies the maximum line length for
 * block comments. This option does not apply to comments to the right of the
 * code though, or to the code itself.
 */

#indent input
void
example(int a, int b, int c, const char *cp)
{
	for (const char *p = cp; *p != '\0'; p++)
		if (*p > a)
			if (*p > b)
				if (*p > c)
					return;

	function(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
}
#indent end

#indent run-equals-input -lc38
