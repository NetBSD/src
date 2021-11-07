/* $NetBSD: opt_bl_br.c,v 1.2 2021/11/07 19:18:56 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void
example(int n)
{
	/*
	 * XXX: The '} else' looks strange in this style since the 'else' is
	 * not at the left margin of the code.
	 */
	if (n > 99) { print("large"); }
	else if (n > 9) { print("double-digit"); }
	else if (n > 0) print("positive");
	else { print("negative"); }
}
#indent end

#indent run -bl
void
example(int n)
{
	/*
	 * XXX: The '} else' looks strange in this style since the 'else' is
	 * not at the left margin of the code.
	 */
	if (n > 99)
	{
		print("large");
	} else if (n > 9)
	{
		print("double-digit");
	} else if (n > 0)
		print("positive");
	else
	{
		print("negative");
	}
}
#indent end

#indent input
void
example(int n)
{
	if (n > 99) { print("large"); }
	else if (n > 9) { print("double-digit"); }
	else if (n > 0) print("positive");
	else { print("negative"); }
}
#indent end

#indent run -br
void
example(int n)
{
	if (n > 99) {
		print("large");
	} else if (n > 9) {
		print("double-digit");
	} else if (n > 0)
		print("positive");
	else {
		print("negative");
	}
}
#indent end


/*
 * Test C99 comments after 'if (expr)', which is handled by search_stmt.
 */
#indent input
void function(void)
{
	if (expr) // C99 comment
		stmt();

	if (expr) // C99 comment
	{
		stmt();
	}
}
#indent end

#indent run
void
function(void)
{
	if (expr)		// C99 comment
		stmt();

	if (expr) {		// C99 comment
		stmt();
	}
}
#indent end


/*
 * Test multiple mixed comments after 'if (expr)'.
 */
#indent input
void
function(void)
{
	if (expr)	// C99 comment 1
			// C99 comment 2
			// C99 comment 3
		stmt();
}
#indent end

#indent run
void
function(void)
{
	if (expr)		// C99 comment 1
		// C99 comment 2
		// C99 comment 3
		stmt();
}
#indent end
