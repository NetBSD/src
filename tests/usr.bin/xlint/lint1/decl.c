/*	$NetBSD: decl.c,v 1.15 2022/04/24 20:08:23 rillig Exp $	*/
# 3 "decl.c"

/*
 * Tests for declarations, especially the distinction between the
 * declaration-specifiers and the declarators.
 */

/*
 * Even though 'const' comes after 'char' and is therefore quite close to the
 * first identifier, it applies to both identifiers.
 */
void
specifier_qualifier(void)
{
	char const a = 1, b = 2;

	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	a = 1;
	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	b = 2;
}

/*
 * Since 'const' comes before 'char', there is no ambiguity whether the
 * 'const' applies to all variables or just to the first.
 */
void
qualifier_specifier(void)
{
	const char a = 1, b = 2;

	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	a = 3;
	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	b = 5;
}

void
declarator_with_prefix_qualifier(void)
{
	/* expect+1: syntax error 'const' [249] */
	char a = 1, const b = 2;

	a = 1;
	/* expect+1: error: 'b' undefined [99] */
	b = 2;
}

void
declarator_with_postfix_qualifier(void)
{
	/* expect+1: syntax error 'const' [249] */
	char a = 1, b const = 2;

	a = 1;
	b = 2;
}

void sink(double *);

void
declarators(void)
{
	char *pc = 0, c = 0, **ppc = 0;

	/* expect+1: warning: converting 'pointer to char' to incompatible 'pointer to double' */
	sink(pc);
	/* expect+1: warning: illegal combination of pointer (pointer to double) and integer (char) */
	sink(c);
	/* expect+1: converting 'pointer to pointer to char' to incompatible 'pointer to double' */
	sink(ppc);
}

_Bool
enum_error_handling(void)
{
	enum {
		/* expect+1: syntax error '"' [249] */
		"error 1"
		:		/* still the same error */
		,		/* back on track */
		A,
		B
	} x = A;

	return x == B;
}

/*
 * An __attribute__ at the beginning of a declaration may become ambiguous
 * since a GCC fallthrough statement starts with __attribute__ as well.
 */
void
unused_local_variable(void)
{
	__attribute__((unused)) _Bool unused_var;

	__attribute__((unused))
	__attribute__((unused)) _Bool unused_twice;
}

int
declaration_without_type_specifier(void)
{
	const i = 3;
	/* expect-1: error: old style declaration; add 'int' [1] */
	return i;
}

/* TODO: add quotes around %s */
/* expect+2: warning: static function unused unused [236] */
static void
unused(void)
{
}

/*
 * The attribute 'used' does not influence static functions, it only
 * applies to function parameters.
 */
/* LINTED */
static void
unused_linted(void)
{
}

/* covers 'type_qualifier_list: type_qualifier_list type_qualifier' */
int *const volatile cover_type_qualifier_list;

_Bool bool;
char plain_char;
signed char signed_char;
unsigned char unsigned_char;
short signed_short;
unsigned short unsigned_short;
int signed_int;
unsigned int unsigned_int;
long signed_long;
unsigned long unsigned_long;
struct {
	int member;
} unnamed_struct;

/*
 * Before decl.c 1.201 from 2021-07-15, lint crashed with an internal error
 * in end_type.
 */
unsigned long sizes =
    sizeof(const typeof(bool)) +
    sizeof(const typeof(plain_char)) +
    sizeof(const typeof(signed_char)) +
    sizeof(const typeof(unsigned_char)) +
    sizeof(const typeof(signed_short)) +
    sizeof(const typeof(unsigned_short)) +
    sizeof(const typeof(signed_int)) +
    sizeof(const typeof(unsigned_int)) +
    sizeof(const typeof(signed_long)) +
    sizeof(const typeof(unsigned_long)) +
    sizeof(const typeof(unnamed_struct));

/* expect+2: error: old style declaration; add 'int' [1] */
/* expect+1: syntax error 'int' [249] */
thread int thread_int;
__thread int thread_int;
/* expect+2: error: old style declaration; add 'int' [1] */
/* expect+1: syntax error 'int' [249] */
__thread__ int thread_int;

/* expect+4: error: old style declaration; add 'int' [1] */
/* expect+2: warning: static function cover_func_declarator unused [236] */
static
cover_func_declarator(void)
{
}

/*
 * Before decl.c 1.268 from 2022-04-03, lint ran into an assertion failure for
 * "elsz > 0" in 'length'.
 */
/* expect+2: error: syntax error 'goto' [249] */
/* expect+1: warning: empty array declaration: void_array_error [190] */
void void_array_error[] goto;
