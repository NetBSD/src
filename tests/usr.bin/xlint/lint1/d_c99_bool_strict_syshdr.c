/*	$NetBSD: d_c99_bool_strict_syshdr.c,v 1.6 2021/01/23 23:11:40 rillig Exp $	*/
# 3 "d_c99_bool_strict_syshdr.c"

/*
 * Macros from system headers may use int expressions where bool expressions
 * are expected.  These headers are not allowed to include <stdbool.h>
 * themselves, and even if they could, lint must accept other scalar types
 * as well, since system headers are not changed lightheartedly.
 */

/* lint1-extra-flags: -T */

extern const unsigned short *ctype_table;

extern void println(const char *);

/*
 * On NetBSD 8, <sys/select.h> defines FD_ISSET by enclosing the statements
 * in the well-known 'do { ... } while (CONSTCOND 0)' loop.  The 0 in the
 * controlling expression has type INT but should be allowed nevertheless
 * since that header does not have a way to distinguish between bool and int.
 * It just follows the C99 standard, unlike the lint-provided stdbool.h, which
 * redefines 'false' to '__lint_false'.  Plus, <sys/select.h> must not include
 * <stdbool.h> itself.
 */
void
strict_bool_system_header_statement_macro(void)
{

	do {
		println("nothing");
	} while (/*CONSTCOND*/0);	/* expect: 333 */

# 35 "d_c99_bool_strict_syshdr.c" 3 4
	do {
		println("nothing");
	} while (/*CONSTCOND*/0);	/* ok */

# 40 "d_c99_bool_strict_syshdr.c"
	do {
		println("nothing");
	} while (/*CONSTCOND*/0);	/* expect: 333 */
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
# 74 "d_c99_bool_strict_syshdr.c" 3 4
	    (int)((ctype_table + 1)[c] & 0x0040)	/* INT */
# 76 "d_c99_bool_strict_syshdr.c"
	;			/* expect: 107 */

	int system_bool_assigned_to_int =
# 80 "d_c99_bool_strict_syshdr.c" 3 4
	    (int)((ctype_table + 1)[c] & 0x0040) != 0	/* BOOL */
# 82 "d_c99_bool_strict_syshdr.c"
	;

	if (
# 86 "d_c99_bool_strict_syshdr.c" 3 4
	    (int)((ctype_table + 1)[c] & 0x0040)	/* INT */
# 88 "d_c99_bool_strict_syshdr.c"
	)
		println("system macro returning INT");

	if (
# 93 "d_c99_bool_strict_syshdr.c" 3 4
	    ((ctype_table + 1)[c] & 0x0040) != 0	/* BOOL */
# 95 "d_c99_bool_strict_syshdr.c"
	)
		println("system macro returning BOOL");
}

static inline _Bool
ch_isspace_sys_int(char c)
{
	return
# 104 "d_c99_bool_strict_syshdr.c" 3 4
	    ((ctype_table + 1)[c] & 0x0040)
# 106 "d_c99_bool_strict_syshdr.c"
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
# 119 "d_c99_bool_strict_syshdr.c" 3 4
	    ((ctype_table + 1)[(unsigned char)c] & 0x0040) != 0
# 121 "d_c99_bool_strict_syshdr.c"
	    != 0;
}

/*
 * If a function from a system header has return type int, which has
 * traditionally been used for the missing type bool, it may be used
 * in controlling expressions.
 */

# 1 "math.h" 3 4
extern int finite(double);
# 133 "d_c99_bool_strict_syshdr.c"

_Bool
call_finite(double d)		/*FIXME*//* expect: 231 */
{
	return finite(d);	/*FIXME*//* expect: 211 */
}
