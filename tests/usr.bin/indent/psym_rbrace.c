/* $NetBSD: psym_rbrace.c,v 1.5 2023/06/16 23:19:01 rillig Exp $ */

/*
 * Tests for the parser symbol psym_rbrace, which represents '}' and finishes
 * the previous '{'.
 *
 * See also:
 *	psym_lbrace.c
 */


/*
 * While it is a syntax error to have an unfinished declaration between braces,
 * indent is forgiving enough to accept this input.
 */
//indent input
{
	int
}
//indent end

//indent run
{
	int
	}
// exit 1
// error: Standard Input:3: Statement nesting error
// error: Standard Input:3: Stuff missing from end of file
//indent end


//indent input
{
	do {
	} while (cond)
}
//indent end

// XXX: Why doesn't indent complain about the missing semicolon?
//indent run-equals-input


//indent input
{
	if (cond)
}
//indent end

//indent run
{
	if (cond)
		}
// exit 1
// error: Standard Input:3: Statement nesting error
// error: Standard Input:3: Stuff missing from end of file
//indent end


//indent input
{
	switch (expr)
}
//indent end

//indent run
{
	switch (expr)
		}
// exit 1
// error: Standard Input:3: Statement nesting error
// error: Standard Input:3: Stuff missing from end of file
//indent end


//indent input
{
	while (cond)
}
//indent end

//indent run
{
	while (cond)
		}
// exit 1
// error: Standard Input:3: Statement nesting error
// error: Standard Input:3: Stuff missing from end of file
//indent end
