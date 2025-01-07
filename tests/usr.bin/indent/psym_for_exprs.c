/* $NetBSD: psym_for_exprs.c,v 1.7 2025/01/07 03:14:24 rillig Exp $ */

/*
 * Tests for the parser state psym_for_exprs, which represents the state after
 * reading the keyword 'for' and the 3 expressions, now waiting for the body
 * of the loop.
 */

//indent input
void
for_loops(void)
{
	int i;

	for (i = 0; i < 10; i++)
		printf("%d * %d = %d\n", i, 7, i * 7);
	for (i = 0; i < 10; i++) {
		printf("%d * %d = %d\n", i, 7, i * 7);
	}

	for (int j = 0; j < 10; j++)
		printf("%d * %d = %d\n", j, 7, j * 7);
	for (int j = 0; j < 10; j++) {
		printf("%d * %d = %d\n", j, 7, j * 7);
	}
}
//indent end

//indent run-equals-input -ldi0


//indent input
{
for (ever1)
for (ever2)
for (ever3)
return;

stmt;
}
//indent end

//indent run
{
	for (ever1)
		for (ever2)
			for (ever3)
				return;

	stmt;
}
//indent end
