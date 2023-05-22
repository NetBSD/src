/* $NetBSD: edge_cases.c,v 1.2 2023/05/22 23:01:27 rillig Exp $ */

/*
 * Tests for edge cases in the C programming language that indent does not
 * support or in which cases indent behaves strangely.
 */

/*
 * Digraphs are replacements for the characters '[', '{' and '#', which are
 * missing in some exotic restricted source character sets.  They are not used
 * in practice, therefore indent doesn't need to support them.
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
// $ Indent interprets everything before the second ':' as a label name,
// $ therefore the statement is indented that far to the left.
// $
// $ The space between 'array' and '<' comes from the binary operator '<'.
number = array <:subscript:>;

	/* same as '(int){ initializer }' */
// $ The opening '<' and '%' are interpreted as unary operators.
// $ The closing '%' and '>' are interpreted as a binary and unary operator.
	number = (int)<%initializer % >;
}
//indent end

/* TODO: test trigraphs, which are as unusual as digraphs */
/* TODO: test digraphs and trigraphs in string literals, just for fun */
