/* $NetBSD: opt_bl_br.c,v 1.9 2023/05/21 10:18:44 rillig Exp $ */

//indent input
void
standard_style(int n)
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

//indent run-equals-input -br

//indent run -bl
void
standard_style(int n)
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


/*
 * In this very condensed style, the additional newline between '}' and 'else'
 * is kept.
 */
//indent input
void
condensed_style(int n)
{
	if (n > 99) { print("large"); }
	else if (n > 9) { print("double-digit"); }
	else if (n > 0) print("positive");
	else { print("negative"); }
}
//indent end

//indent run -bl
void
condensed_style(int n)
{
	if (n > 99)
	{
		print("large");
	}
	else if (n > 9)
	{
		print("double-digit");
	}
	else if (n > 0)
		print("positive");
	else
	{
		print("negative");
	}
}
//indent end

//indent run -br
void
condensed_style(int n)
{
	if (n > 99) {
		print("large");
	}
	else if (n > 9) {
		print("double-digit");
	}
	else if (n > 0)
		print("positive");
	else {
		print("negative");
	}
}
//indent end


/*
 * An end-of-line comment after 'if (expr)' forces the '{' to go to the next
 * line.
 */
//indent input
void
eol_comment(void)
{
	if (expr) // C99 comment
		stmt();

	if (expr) // C99 comment
	{
		stmt();
	}
}
//indent end

//indent run -br
void
eol_comment(void)
{
	if (expr)		// C99 comment
		stmt();

	if (expr)		// C99 comment
	{
		stmt();
	}
}
//indent end

//indent run-equals-prev-output -bl


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
 * removes extra newlines between the tokens '}', 'else' and 'if'.
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

/* TODO: Remove the newline between ')' and '{'. */
//indent run-equals-input -br


//indent input
void
comments(void)
{
	if(cond){}

	if (cond)
	{}

	if (cond) /* comment */
	{}

	if (cond)
	/* comment */
	{}

	if (cond)
	// comment1
	// comment2
	{}

	if (cond) // comment
	{}
}
//indent end

//indent run -bl
void
comments(void)
{
	if (cond)
	{
	}

	if (cond)
	{
	}

	if (cond)		/* comment */
	{
	}

	if (cond)
		/* comment */
	{
	}

	if (cond)
		// comment1
		// comment2
	{
	}

	if (cond)		// comment
	{
	}
}
//indent end

//indent run -br
void
comments(void)
{
	if (cond) {
	}

	if (cond)
	{
	}

	if (cond)		/* comment */
	{
	}

	if (cond)
		/* comment */
	{
	}

	if (cond)
		// comment1
		// comment2
	{
	}

	if (cond)		// comment
	{
	}
}
//indent end
