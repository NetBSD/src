/* $NetBSD: token_form_feed.c,v 1.4 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for form feeds, which is a control character (C99 5.2.1p3).
 */

#indent input
void function_1(void);

void function_2(void);
#indent end

#indent run -di0
void function_1(void);

/* $ XXX: The form feed is not preserved. */
/* $ XXX: Why 2 empty lines? */

void function_2(void);
#indent end


/*
 * Test form feed after 'if (expr)', which is handled in search_stmt.
 */
#indent input
void function(void)
{
	if (expr)
	 /* <-- form feed */
	{
	}
}
#indent end

#indent run
void
function(void)
{
	if (expr) {
		/* $ XXX: The form feed has disappeared. */
		/* <-- form feed */
	}
}
#indent end
