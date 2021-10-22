/* $NetBSD: opt_ts.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the option '-ts', which specifies the width of a single
 * tabulator.
 */

/*
 * Since in this test, a tab is only worth 4 spaces, the indentation needs
 * more tabs to reach the indentation of 8 and the alignment at 16.
 */
#indent input
int variable;

void
function(void)
{
	int local_variable;
	char local_variable;
}
#indent end

#indent run -ts4
int				variable;

void
function(void)
{
		int				local_variable;
		char			local_variable;
}
#indent end
