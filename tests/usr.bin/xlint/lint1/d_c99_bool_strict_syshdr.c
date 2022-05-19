/*	$NetBSD: d_c99_bool_strict_syshdr.c,v 1.13 2022/05/19 17:10:50 rillig Exp $	*/
# 3 "d_c99_bool_strict_syshdr.c"

/*
 * In strict bool mode, lint treats bool as incompatible with any other scalar
 * types.  This mode helps in migrating code from pre-C99 to C99.
 *
 * System headers, on the other hand, cannot be migrated if they need to stay
 * compatible with pre-C99 code.  Therefore, the checks for system headers are
 * loosened.  In contexts where a scalar expression is compared to 0, macros
 * and functions from system headers may use int expressions as well.
 */

/* lint1-extra-flags: -T */

extern const unsigned short *ctype_table;

extern void println(const char *);

/*
 * On NetBSD 8, <sys/select.h> defines FD_ISSET by enclosing the statements
 * in the well-known 'do { ... } while (CONSTCOND 0)' loop.  The 0 in the
 * controlling expression has type INT but should be allowed nevertheless
 * since that header does not have a way to distinguish between bool and int.
 * It just follows the C99 standard, unlike the lint-provided stdbool.h,
 * which redefines 'false' to '__lint_false'.
 */
void
strict_bool_system_header_statement_macro(void)
{

	do {
		println("nothing");
	} while (/*CONSTCOND*/0);
	/* expect-1: error: controlling expression must be bool, not 'int' [333] */

# 38 "d_c99_bool_strict_syshdr.c" 3 4
	do {
		println("nothing");
	} while (/*CONSTCOND*/0);	/* ok */

# 43 "d_c99_bool_strict_syshdr.c"
	do {
		println("nothing");
	} while (/*CONSTCOND*/0);
	/* expect-1: error: controlling expression must be bool, not 'int' [333] */
}


/*
 * The macros from <ctype.h> can be implemented in different ways.  The C
 * standard defines them as returning 'int'.  In strict bool mode, the actual
 * return type can be INT or BOOL, depending on whether the macros do the
 * comparison against 0 themselves.
 *
 * Since that comparison is more code to write and in exceptional situations
 * more code to execute, they will probably leave out the extra comparison,
 * but both ways are possible.
 *
 * In strict bool mode, there must be a way to call these function-like macros
 * portably, without triggering type errors, no matter whether they return
 * BOOL or INT.
 *
 * The expressions from this example cross the boundary between system header
 * and application code.  They need to carry the information that they are
 * half-BOOL, half-INT across to the enclosing expressions.
 */
void
strict_bool_system_header_ctype(int c)
{
	/*
	 * The macro returns INT, which may be outside the range of a
	 * uint8_t variable, therefore it must not be assigned directly.
	 * All other combinations of type are safe from truncation.
	 */
	_Bool system_int_assigned_to_bool =
# 78 "d_c99_bool_strict_syshdr.c" 3 4
	    (int)((ctype_table + 1)[c] & 0x0040)	/* INT */
# 80 "d_c99_bool_strict_syshdr.c"
	;
	/* expect-1: error: operands of 'init' have incompatible types (_Bool != int) [107] */

	int system_bool_assigned_to_int =
# 85 "d_c99_bool_strict_syshdr.c" 3 4
	    (int)((ctype_table + 1)[c] & 0x0040) != 0	/* BOOL */
# 87 "d_c99_bool_strict_syshdr.c"
	;

	if (
# 91 "d_c99_bool_strict_syshdr.c" 3 4
	    (int)((ctype_table + 1)[c] & 0x0040)	/* INT */
# 93 "d_c99_bool_strict_syshdr.c"
	)
		println("system macro returning INT");

	if (
# 98 "d_c99_bool_strict_syshdr.c" 3 4
	    ((ctype_table + 1)[c] & 0x0040) != 0	/* BOOL */
# 100 "d_c99_bool_strict_syshdr.c"
	)
		println("system macro returning BOOL");
}

static inline _Bool
ch_isspace_sys_int(char c)
{
	return
# 109 "d_c99_bool_strict_syshdr.c" 3 4
	    ((ctype_table + 1)[c] & 0x0040)
# 111 "d_c99_bool_strict_syshdr.c"
	    != 0;
}

/*
 * isspace is defined to return an int.  Comparing this int with 0 is the
 * safe way to convert it to _Bool.  This must be allowed even if isspace
 * does the comparison itself.
 */
static inline _Bool
ch_isspace_sys_bool(char c)
{
	return
# 124 "d_c99_bool_strict_syshdr.c" 3 4
	    ((ctype_table + 1)[(unsigned char)c] & 0x0040) != 0
# 126 "d_c99_bool_strict_syshdr.c"
	    != 0;
}

/*
 * There are several functions from system headers that have return type
 * int.  For this return type there are many API conventions:
 *
 *	* isspace: 0 means no, non-zero means yes
 *	* open: 0 means success, -1 means failure
 *	* main: 0 means success, non-zero means failure
 *	* strcmp: 0 means equal, < 0 means less than, > 0 means greater than
 *
 * Without a detailed list of individual functions, it's not possible to
 * guess what the return value means.  Therefore in strict bool mode, the
 * return value of these functions cannot be implicitly converted to bool,
 * not even in a context where the result is compared to 0.  Allowing that
 * would allow expressions like !strcmp(s1, s2), which is not correct since
 * strcmp returns an "ordered comparison result", not a bool.
 */

# 1 "math.h" 3 4
extern int finite(double);
# 1 "string.h" 3 4
extern int strcmp(const char *, const char *);
# 151 "d_c99_bool_strict_syshdr.c"

/*ARGSUSED*/
_Bool
call_finite_bad(double d)
{
	/* expect+1: error: return value type mismatch (_Bool) and (int) [211] */
	return finite(d);
}

_Bool
call_finite_good(double d)
{
	return finite(d) != 0;
}

/*ARGSUSED*/
_Bool
str_equal_bad(const char *s1, const char *s2)
{
	/* expect+2: error: operand of '!' must be bool, not 'int' [330] */
	/* expect+1: warning: function 'str_equal_bad' expects to return value [214] */
	return !strcmp(s1, s2);
}

_Bool
str_equal_good(const char *s1, const char *s2)
{
	return strcmp(s1, s2) == 0;
}


int read_char(void);

/*
 * Between tree.c 1.395 from 2021-11-16 and ckbool.c 1.10 from 2021-12-22,
 * lint wrongly complained that the controlling expression would have to be
 * _Bool instead of int.  Since the right-hand side of the ',' operator comes
 * from a system header, this is OK though.
 */
void
controlling_expression_with_comma_operator(void)
{
	int c;

	while (c = read_char(),
# 197 "d_c99_bool_strict_syshdr.c" 3 4
	    ((int)((ctype_table + 1)[(
# 199 "d_c99_bool_strict_syshdr.c"
		c
# 201 "d_c99_bool_strict_syshdr.c" 3 4
	    )] & 0x0040 /* Space     */))
# 203 "d_c99_bool_strict_syshdr.c"
	    )
		continue;
}


void take_bool(_Bool);

/*
 * On NetBSD, the header <curses.h> defines TRUE or FALSE as integer
 * constants with a CONSTCOND comment.  This comment suppresses legitimate
 * warnings in user code; that's irrelevant for this test though.
 *
 * Several curses functions take bool as a parameter, for example keypad or
 * leaveok.  As of 2022-05-19, lint does not complain when these functions
 * get 0 instead of 'false' as an argument.  It does complain about 1 instead
 * of 'true' though.
 */
void
pass_bool_to_function(void)
{

	/* expect+5: error: argument #1 expects '_Bool', gets passed 'int' [334] */
	take_bool(
# 227 "d_c99_bool_strict_syshdr.c" 3 4
	    (/*CONSTCOND*/1)
# 229 "d_c99_bool_strict_syshdr.c"
	);

	take_bool(
# 233 "d_c99_bool_strict_syshdr.c" 3 4
	    __lint_true
# 235 "d_c99_bool_strict_syshdr.c"
	);

	/* TODO: make this a type error */
	take_bool(
# 240 "d_c99_bool_strict_syshdr.c" 3 4
	    (/*CONSTCOND*/0)
# 242 "d_c99_bool_strict_syshdr.c"
	);

	take_bool(
# 246 "d_c99_bool_strict_syshdr.c" 3 4
	    __lint_false
# 248 "d_c99_bool_strict_syshdr.c"
	);
}
