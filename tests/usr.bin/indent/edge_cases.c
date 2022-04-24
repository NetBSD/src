/* $NetBSD: edge_cases.c,v 1.1 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for edge cases in the C programming language that indent does not
 * support or in which cases indent behaves strangely.
 */

/*
 * Digraphs are replacements for the characters '[', '{' and '#', which are
 * missing in some exotic restricted source character sets.
 *
 * See C99 6.4.6
 */
//indent input
void
digraphs(void)
{
	/* same as 'array[subscript]' */
	number = array<:subscript:>;

	/* same as '(int){ initializer }' */
	number = (int)<% initializer %>;
}
//indent end

//indent run
void
digraphs(void)
{
	/* same as 'array[subscript]' */
// $ XXX: The indentation is completely wrong.
// $ XXX: The space between 'array' and '<' doesn't belong there.
number = array <:subscript:>;

	/* same as '(int){ initializer }' */
// $ XXX: The space between '%' and '>' doesn't belong there.
	number = (int)<%initializer % >;
}
//indent end

/* TODO: test trigraphs, which are as unusual as digraphs */
/* TODO: test digraphs and trigraphs in string literals, just for fun */
