/* $NetBSD: token_ident.c,v 1.7 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for identifiers, numbers and string literals.
 */

/* TODO: Add more systematic tests. */
/* TODO: Completely cover each state transition in lex_number_state. */

/* Binary number literals, a GCC extension that was added in C11. */
//indent input
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
//indent end

//indent run
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
//indent end


/* Floating point numbers. */
//indent input
void t(void) {
	unsigned long x = 314UL;
	double y[] = {0x1P+9F, 0.3, .1, 1.2f, 0xa.p01f, 3.14f, 2.L};
	int z = 0b0101;
	DO_NOTHING;
	x._y = 5;
}
//indent end

//indent run
void
t(void)
{
	unsigned long	x = 314UL;
	double		y[] = {0x1P+9F, 0.3, .1, 1.2f, 0xa.p01f, 3.14f, 2.L};
	int		z = 0b0101;
	DO_NOTHING;
	x._y = 5;
}
//indent end


/*
 * Test identifiers containing '$', which some compilers support as an
 * extension to the C standard.
 */
//indent input
int $		= jQuery;			// just kidding
const char SYS$LOGIN[]="$HOME";
//indent end

//indent run
int		$ = jQuery;	// just kidding
const char	SYS$LOGIN[] = "$HOME";
//indent end


/*
 * Test the tokenizer for number constants.
 *
 * When the tokenizer reads a character that makes a token invalid (such as
 * '0x') but may later be extended to form a valid token (such as '0x123'),
 * indent does not care about this invalid prefix and returns it nevertheless.
 */
//indent input
int unfinished_hex_prefix = 0x;
double unfinished_hex_float = 0x123p;
//indent end

//indent run-equals-input -di0
