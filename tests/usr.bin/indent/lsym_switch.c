/* $NetBSD: lsym_switch.c,v 1.5 2023/06/14 11:18:09 rillig Exp $ */

/*
 * Tests for the token lsym_switch, which represents the keyword 'switch' that
 * starts a 'switch' statement.
 *
 * See also:
 *	C11 6.8.4		"Selection statements"
 *	C11 6.8.4.2		"The 'switch' statement"
 */

// TODO: Add systematic tests.

/*
 * Ensure that an unfinished 'switch' statement does not eat comments.
 */
//indent input
{
	switch (expr)		// comment
	{
	}
}
//indent end

//indent run-equals-input
