/* $NetBSD: lsym_form_feed.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token lsym_form_feed, which represents a form feed, a special
 * kind of whitespace that is seldom used.  If it is used, it usually appears
 * on a line of its own, after an external-declaration, to force a page break
 * when printing the source code on actual paper.
 */

//indent input
void function_1(void);

void function_2(void);
//indent end

//indent run -di0
void function_1(void);

/* $ XXX: The form feed is not preserved. */
/* $ XXX: Why 2 empty lines? */

void function_2(void);
//indent end


/*
 * Test form feed after 'if (expr)', which is handled in search_stmt.
 */
//indent input
void function(void)
{
	if (expr)
	 /* <-- form feed */
	{
	}
}
//indent end

//indent run
void
function(void)
{
	if (expr) {
		/* $ XXX: The form feed has disappeared. */
		/* <-- form feed */
	}
}
//indent end
