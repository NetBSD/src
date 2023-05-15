/* $NetBSD: psym_for_exprs.c,v 1.5 2023/05/15 08:22:23 rillig Exp $ */

/*
 * Tests for the parser state psym_for_exprs, which represents the state after
 * reading the keyword 'for' and the 3 expressions, now waiting for the body
 * of the loop.
 */

//indent input
void
for_loops(void)
{
	for (int i = 0; i < 10; i++)
		printf("%d * %d = %d\n", i, 7, i * 7);

	for (int i = 0; i < 10; i++) {
		printf("%d * %d = %d\n", i, 7, i * 7);
	}
}
//indent end

//indent run
void
for_loops(void)
{
	for (int i = 0; i < 10; i++)
		printf("%d * %d = %d\n", i, 7, i * 7);

	/* $ FIXME: Add space between ')' and '{'. */
	for (int i = 0; i < 10; i++){
		printf("%d * %d = %d\n", i, 7, i * 7);
	}
}
//indent end


/*
 * Since C99, the first expression of a 'for' loop may be a declaration, not
 * only an expression.
 */
//indent input
void
small_scope(void)
{
	for (int i = 0; i < 3; i++)
		stmt();
}
//indent end

//indent run-equals-input
