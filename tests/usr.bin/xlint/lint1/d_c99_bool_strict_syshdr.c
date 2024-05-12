/*	$NetBSD: d_c99_bool_strict_syshdr.c,v 1.24 2024/05/12 12:28:35 rillig Exp $	*/
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

/* lint1-extra-flags: -T -X 351 */

extern const unsigned short *ctype_table;

extern void println(const char *);




/*
 * No matter whether the code is from a system header or not, the idiom
 * 'do { ... } while (0)' is well known, and using the integer constant 0
 * instead of the boolean constant 'false' neither creates any type confusion
 * nor does its value take place in any conversions, as its scope is limited
 * to the controlling expression of the loop.
 */
void
statement_macro(void)
{

	do {
		println("nothing");
	} while (/*CONSTCOND*/0);

# 39 "d_c99_bool_strict_syshdr.c" 3 4
	do {
		println("nothing");
	} while (/*CONSTCOND*/0);

# 44 "d_c99_bool_strict_syshdr.c"
	do {
		println("nothing");
	} while (/*CONSTCOND*/0);
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
	/* expect-1: error: operands of 'init' have incompatible types '_Bool' and 'int' [107] */

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
 * guess what the return value means.  Therefore, in strict bool mode, the
 * return value of these functions cannot be implicitly converted to bool,
 * not even in a controlling expression.  Allowing that would allow
 * expressions like !strcmp(s1, s2), which is not correct since strcmp
 * returns an "ordered comparison result", not a bool.
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
	/* expect+1: error: function has return type '_Bool' but returns 'int' [211] */
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
	/* expect+1: error: function 'str_equal_bad' expects to return value [214] */
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
 * leaveok.  Before ckbool.c 1.14 from 2022-05-19, lint did not complain when
 * these functions get 0 instead of 'false' as an argument.  It did complain
 * about 1 instead of 'true' though.
 */
void
pass_bool_to_function(void)
{

	/* expect+5: error: parameter 1 expects '_Bool', gets passed 'int' [334] */
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

	/* expect+5: error: parameter 1 expects '_Bool', gets passed 'int' [334] */
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


extern int *errno_location(void);

/*
 * As of 2022-06-11, the rule for loosening the strict boolean check for
 * expressions from system headers is flawed.  That rule allows statements
 * like 'if (NULL)' or 'if (errno)', even though these have pointer type or
 * integer type.
 */
void
if_pointer_or_int(void)
{
	/* if (NULL) */
	if (
# 266 "d_c99_bool_strict_syshdr.c" 3 4
	    ((void *)0)
# 268 "d_c99_bool_strict_syshdr.c"
		       )
		return;
	/* expect-1: warning: statement not reached [193] */

	/* if (EXIT_SUCCESS) */
	if (
# 275 "d_c99_bool_strict_syshdr.c" 3 4
	    0
# 277 "d_c99_bool_strict_syshdr.c"
		       )
		return;
	/* expect-1: warning: statement not reached [193] */

	/* if (errno) */
	if (
# 284 "d_c99_bool_strict_syshdr.c" 3 4
	    (*errno_location())
# 286 "d_c99_bool_strict_syshdr.c"
		       )
		return;
}


/*
 * For expressions that originate from a system header, the strict type rules
 * are relaxed a bit, to allow for expressions like 'flags & FLAG', even
 * though they are not strictly boolean.
 *
 * This shouldn't apply to function call expressions though since one of the
 * goals of strict bool mode is to normalize all expressions calling 'strcmp'
 * to be of the form 'strcmp(a, b) == 0' instead of '!strcmp(a, b)'.
 */
# 1 "stdio.h" 1 3 4
typedef struct stdio_file {
	int fd;
} FILE;
int ferror(FILE *);
FILE stdio_files[3];
FILE *stdio_stdout;
# 308 "d_c99_bool_strict_syshdr.c" 2
# 1 "string.h" 1 3 4
int strcmp(const char *, const char *);
# 311 "d_c99_bool_strict_syshdr.c" 2

void
controlling_expression(FILE *f, const char *a, const char *b)
{
	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(f))
		return;
	/* expect+1: error: controlling expression must be bool, not 'int' [333] */
	if (strcmp(a, b))
		return;
	/* expect+1: error: operand of '!' must be bool, not 'int' [330] */
	if (!ferror(f))
		return;
	/* expect+1: error: operand of '!' must be bool, not 'int' [330] */
	if (!strcmp(a, b))
		return;

	/*
	 * Before tree.c 1.395 from 2021-11-16, the expression below didn't
	 * produce a warning since the expression 'stdio_files' came from a
	 * system header (via a macro), and this property was passed up to
	 * the expression 'ferror(stdio_files[1])'.
	 *
	 * That was wrong though since the type of a function call expression
	 * only depends on the function itself but not its arguments types.
	 * The old rule had allowed a raw condition 'strcmp(a, b)' without
	 * the comparison '!= 0', as long as one of its arguments came from a
	 * system header.
	 *
	 * Seen in bin/echo/echo.c, function main, call to ferror.
	 */
	/* expect+5: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(
# 345 "d_c99_bool_strict_syshdr.c" 3 4
	    &stdio_files[1]
# 347 "d_c99_bool_strict_syshdr.c"
	    ))
		return;

	/*
	 * Before cgram.y 1.369 from 2021-11-16, at the end of parsing the
	 * name 'stdio_stdout', the parser already looked ahead to the next
	 * token, to see whether it was the '(' of a function call.
	 *
	 * At that point, the parser was no longer in a system header,
	 * therefore 'stdio_stdout' had tn_sys == false, and this information
	 * was pushed down to the whole function call expression (which was
	 * another bug that got fixed in tree.c 1.395 from 2021-11-16).
	 */
	/* expect+5: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(
# 363 "d_c99_bool_strict_syshdr.c" 3 4
	    stdio_stdout
# 365 "d_c99_bool_strict_syshdr.c"
	    ))
		return;

	/*
	 * In this variant of the pattern, there is a token ')' after the
	 * name 'stdio_stdout', which even before tree.c 1.395 from
	 * 2021-11-16 had the effect that at the end of parsing the name, the
	 * parser was still in the system header, thus setting tn_sys (or
	 * rather tn_relaxed at that time) to true.
	 */
	/* expect+5: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(
# 378 "d_c99_bool_strict_syshdr.c" 3 4
	    (stdio_stdout)
# 380 "d_c99_bool_strict_syshdr.c"
	    ))
		return;

	/*
	 * Before cgram.y 1.369 from 2021-11-16, the comment following
	 * 'stdio_stdout' did not prevent the search for '('.  At the point
	 * where build_name called expr_alloc_tnode, the parser was already
	 * in the main file again, thus treating 'stdio_stdout' as not coming
	 * from a system header.
	 *
	 * This has been fixed in tree.c 1.395 from 2021-11-16.  Before that,
	 * an expression had come from a system header if its operands came
	 * from a system header, but that was only close to the truth.  In a
	 * case where both operands come from a system header but the
	 * operator comes from the main translation unit, the main
	 * translation unit still has control over the whole expression.  So
	 * the correct approach is to focus on the operator, not the
	 * operands.  There are a few corner cases where the operator is
	 * invisible (for implicit conversions) or synthetic (for translating
	 * 'arr[index]' to '*(arr + index)', but these are handled as well.
	 */
	/* expect+5: error: controlling expression must be bool, not 'int' [333] */
	if (ferror(
# 404 "d_c99_bool_strict_syshdr.c" 3 4
	    stdio_stdout /* comment */
# 406 "d_c99_bool_strict_syshdr.c"
	    ))
		return;
}
