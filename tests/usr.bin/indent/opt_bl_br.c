/* $NetBSD: opt_bl_br.c,v 1.6 2022/04/24 09:04:12 rillig Exp $ */

//indent input
void
example(int n)
{
	if (n > 99) { print("large"); }
	else if (n > 9) { print("double-digit"); }
	else if (n > 0) print("positive");
	else { print("negative"); }
}
//indent end

/*
 * XXX: The '} else' looks strange in this style since the '}' is not on a
 * line of its own.
 */
//indent run -bl
void
example(int n)
{
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
//indent end

//indent run -br
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
//indent end


/*
 * Test C99 comments after 'if (expr)', which are handled by search_stmt.
 */
//indent input
void function(void)
{
	if (expr) // C99 comment
		stmt();

	if (expr) // C99 comment
	{
		stmt();
	}
}
//indent end

//indent run
void
function(void)
{
	if (expr)		// C99 comment
		stmt();

	if (expr) {		// C99 comment
		stmt();
	}
}
//indent end


/*
 * Test multiple mixed comments after 'if (expr)'.
 */
//indent input
void
function(void)
{
	if (expr)	// C99 comment 1
			// C99 comment 2
			// C99 comment 3
		stmt();
}
//indent end

//indent run
void
function(void)
{
	if (expr)		// C99 comment 1
		// C99 comment 2
		// C99 comment 3
		stmt();
}
//indent end


/*
 * The combination of the options '-br' and '-ei' (both active by default)
 * remove extra newlines between the tokens '}', 'else' and 'if'.
 */
//indent input
void
function(void)
{
	if (cond)
	{
		stmt();
	}
	else
	if (cond)
	{
		stmt();
	}
}
//indent end

//indent run -br
void
function(void)
{
	if (cond) {
		stmt();
	} else if (cond) {
		stmt();
	}
}
//indent end
