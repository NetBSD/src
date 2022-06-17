/*	$NetBSD: lex_integer.c,v 1.10 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "lex_integer.c"

/*
 * Tests for lexical analysis of integer constants.
 *
 * C99 6.4.4.1 "Integer constants"
 */

/* lint1-only-if: lp64 */

long signed_long;
unsigned long long unsigned_long_long_var;

struct s {
	int member;
};
/*
 * When lint tries to convert the argument to 'struct s', it prints the
 * actual type of the argument as a side effect.
 */
void print_type(struct s);

void
no_suffix(void)
{
	/* expect+1: ... passing 'int' ... */
	print_type(0);
	/* The '-' is not part of the constant, it is a unary operator. */
	/* expect+1: ... passing 'int' ... */
	print_type(-1);

	/* expect+1: ... passing 'int' ... */
	print_type(2147483647);
	/* expect+1: ... passing 'int' ... */
	print_type(0x7fffffff);
	/* expect+1: ... passing 'int' ... */
	print_type(017777777777);

	/* expect+1: ... passing 'unsigned int' ... */
	print_type(0x80000000);
	/* expect+1: ... passing 'unsigned int' ... */
	print_type(020000000000);
	/* expect+1: ... passing 'unsigned int' ... */
	print_type(0xffffffff);

	/* expect+1: ... passing 'long' ... */
	print_type(2147483648);
	/* expect+1: ... passing 'long' ... */
	print_type(0x0000000100000000);
	/* expect+1: ... passing 'long' ... */
	print_type(0x7fffffffffffffff);

	/* expect+1: ... passing 'unsigned long' ... */
	print_type(0x8000000000000000);
	/* expect+1: ... passing 'unsigned long' ... */
	print_type(0xffffffffffffffff);

	/* expect+2: warning: integer constant out of range [252] */
	/* expect+1: ... passing 'unsigned long' ... */
	print_type(0x00010000000000000000);
}

void
suffix_u(void)
{
	/* expect+1: ... passing 'unsigned int' ... */
	print_type(3U);
	/* expect+1: ... passing 'unsigned int' ... */
	print_type(3u);

	/* expect+1: ... passing 'unsigned int' ... */
	print_type(4294967295U);
	/* expect+1: ... passing 'unsigned long' ... */
	print_type(4294967296U);
}

void
suffix_l(void)
{
	/* expect+1: ... passing 'long' ... */
	print_type(3L);

	/* expect+1: ... passing 'long' ... */
	print_type(3l);
}

void
suffix_ul(void)
{
	/* expect+1: ... passing 'unsigned long' ... */
	print_type(3UL);
	/* expect+1: ... passing 'unsigned long' ... */
	print_type(3LU);
}

void
suffix_ll(void)
{
	/* expect+1: ... passing 'long long' ... */
	print_type(3LL);

	/* The 'Ll' must not use mixed case. Checked by the compiler. */
	/* expect+1: ... passing 'long long' ... */
	print_type(3Ll);

	/* expect+1: ... passing 'long long' ... */
	print_type(3ll);
}

void
suffix_ull(void)
{
	/* expect+1: ... passing 'unsigned long long' ... */
	print_type(3llu);
	/* expect+1: ... passing 'unsigned long long' ... */
	print_type(3Ull);

	/* The 'LL' must not be split. Checked by the compiler. */
	/* expect+1: ... passing 'unsigned long long' ... */
	print_type(3lul);

	/* The 'Ll' must not use mixed case. Checked by the compiler. */
	/* expect+1: ... passing 'unsigned long long' ... */
	print_type(3ULl);
}

void
suffix_too_many(void)
{
	/* expect+2: warning: malformed integer constant [251] */
	/* expect+1: ... passing 'long long' ... */
	print_type(3LLL);

	/* expect+2: warning: malformed integer constant [251] */
	/* expect+1: ... passing 'unsigned int' ... */
	print_type(3uu);
}

/* https://gcc.gnu.org/onlinedocs/gcc/Binary-constants.html */
void
binary_literal(void)
{
	/* This is a GCC extension, but lint doesn't know that. */
	/* expect+1: ... passing 'int' ... */
	print_type(0b1111000001011010);

	/* expect+1: ... passing 'unsigned int' ... */
	print_type(0b11110000111100001111000011110000);
}
