/* $NetBSD: lsym_do.c,v 1.8 2023/06/05 08:10:25 rillig Exp $ */

/*
 * Tests for the token lsym_do, which represents the keyword 'do' that starts
 * a 'do-while' loop.
 *
 * See also:
 *	psym_do.c
 *	psym_do_stmt.c
 *	C11 6.8.5		"Iteration statements"
 *	C11 6.8.5.2		"The 'do' statement"
 */

//indent input
void
function(void)
{
	do stmt();while(cond);
}
//indent end

//indent run
void
function(void)
{
	do
		stmt();
	while (cond);
}
//indent end


//indent input
void
else_do(int i)
{
	if (i > 0) return; else do {} while (0);
}
//indent end

//indent run
void
else_do(int i)
{
	if (i > 0)
		return;
	else
		do {
		} while (0);
}
//indent end


//indent input
void
variants(void)
{
	do stmt(); while (0);

	do { stmt(); } while (0);

	do /* comment */ stmt(); while (0);

	while (0) do {} while (0);
}
//indent end

//indent run
void
variants(void)
{
	do
		stmt();
	while (0);

	do {
		stmt();
	} while (0);

	do			/* comment */
		stmt();
	while (0);

	while (0)
		do {
		} while (0);
}
//indent end

//indent run -bl
void
variants(void)
{
	do
		stmt();
	while (0);

	do
	{
		stmt();
	} while (0);

	do			/* comment */
		stmt();
	while (0);

	while (0)
		do
		{
		} while (0);
}
//indent end


/* Ensure that the 'do' starts a line. */
//indent input
{
	/* */ do {} while (false);

	word do {} while (false);

	label: do {} while (false);
}
//indent end

//indent run
{
// $ FIXME: Trailing whitespace.
	/* */ 
	do {
	} while (false);

	word
	do {
	} while		(false);

label:	do {
	} while (false);
}
//indent end

//indent run -sob
{
// $ FIXME: Trailing whitespace.
	/* */ 
	do {
	} while (false);
// $ FIXME: This blank line is not optional and must be preserved.
	word
	do {
// $ FIXME: The expression is indented too far to the right.
	} while		(false);
// $ FIXME: This blank line is not optional and must be preserved.
label:	do {
	} while (false);
}
//indent end
