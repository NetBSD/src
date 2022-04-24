/* $NetBSD: lsym_word.c,v 1.6 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token lsym_word, which represents a constant, a string
 * literal or an identifier.
 *
 * See also:
 *	lsym_funcname.c		for an identifier followed by '('
 */

// TODO: Is '"string"(' syntactically valid in any context?
// TODO: Is '123(' syntactically valid in any context?
// TODO: Would the output of the above depend on -pcs/-npcs?
// TODO: Add more systematic tests.
// TODO: Completely cover each state transition in lex_number_state.

//indent input
// TODO: add input
//indent end

//indent run-equals-input


/*
 * Since 2019-04-04 and before NetBSD lexi.c 1.149 from 2021-11-20, the first
 * character after a backslash continuation was always considered part of a
 * word, no matter whether it was a word character or not.
 */
//indent input
int var\
+name = 4;
//indent end

//indent run
int		var + name = 4;
//indent end


//indent input
wchar_t wide_string[] = L"wide string";
//indent end

/*
 * Regardless of the line length, the 'L' must never be separated from the
 * string literal.  Before lexi.c 1.167 from 2021-11-28, the 'L' was a
 * separate token, which could have resulted in accidental spacing between the
 * 'L' and the following "".
 */
//indent run-equals-input -di0

//indent run-equals-input -di0 -l25

//indent run-equals-input -di0 -l1


//indent input
wchar_t wide_char[] = L'w';
//indent end

//indent run-equals-input -di0


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
