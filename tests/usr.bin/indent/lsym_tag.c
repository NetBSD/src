/* $NetBSD: lsym_tag.c,v 1.2 2022/02/12 19:46:56 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_tag, which represents one of the keywords
 * 'struct', 'union' or 'enum' that declare, define or use a tagged type.
 */

/* TODO: Add systematic tests for 'struct'. */
/* TODO: Add systematic tests for 'union'. */
/* TODO: Add systematic tests for 'enum'. */


#indent input
int
indent_enum_constants(void)
{
	enum color {
		red,
		green
	};
	enum color	colors[] = {
		red,
		red,
		red,
	};
	/*
	 * Ensure that the token sequence 'enum type {' only matches if there
	 * are no other tokens in between, to prevent statement continuations
	 * from being indented like enum constant declarations.
	 *
	 * See https://gnats.netbsd.org/55453.
	 */
	if (colors[0] == (enum color)1) {
		return 1
		  + 2
		  + 3;
	}
	return 0;
}
#indent end

#indent run-equals-input -ci2
