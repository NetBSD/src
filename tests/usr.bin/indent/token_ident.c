/* $NetBSD: token_ident.c,v 1.1 2021/10/18 18:10:20 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for identifiers, numbers and string literals.
 */

/* TODO: Add more systematic tests. */
/* TODO: Completely cover each state transition in lex_number_state. */

/* Binary number literals, a GCC extension that was added in C11. */
#indent input
#define b00101010 -1
void t(void) {
	unsigned a[] = {0b00101010, 0x00005678, 02, 17U};
	float x[] = {.7f, 0.7f};
	unsigned long ul[] = {0b00001111UL, 0x01010101UL, 02UL, 17UL};

	if (0 b00101010)
		return;
	/* $ '0r' is not a number base prefix, so the tokens are split. */
	if (0r12345)
		return;
}
#indent end

#indent run
#define b00101010 -1
void
t(void)
{
	unsigned	a[] = {0b00101010, 0x00005678, 02, 17U};
	float		x[] = {.7f, 0.7f};
	unsigned long	ul[] = {0b00001111UL, 0x01010101UL, 02UL, 17UL};

	if (0 b00101010)
		return;
	if (0 r12345)
		return;
}
#indent end

/* Floating point numbers. */
#indent input
void t(void) {
	unsigned long x = 314UL;
	double y[] = {0x1P+9F, 0.3, .1, 1.2f, 0xa.p01f, 3.14f, 2.L};
	int z = 0b0101;
	DO_NOTHING;
	x._y = 5;
}
#indent end

#indent run
void
t(void)
{
	unsigned long	x = 314UL;
	double		y[] = {0x1P+9F, 0.3, .1, 1.2f, 0xa.p01f, 3.14f, 2.L};
	int		z = 0b0101;
	DO_NOTHING;
	x._y = 5;
}
#indent end
