/*	$NetBSD: d_c99_bool_strict_syshdr.c,v 1.2 2021/01/17 23:04:09 rillig Exp $	*/
# 3 "d_c99_bool_strict_syshdr.c"

/*
 * Macros from system headers may use int expressions where bool expressions
 * are expected.  These headers are not allowed to include <stdbool.h>
 * themselves, and even if they could, lint must accept other scalar types
 * as well, since system headers are not changed lightheartedly.
 */

/* lint1-extra-flags: -T */

/*
 * On NetBSD 8, <sys/select.h> defines FD_ISSET by enclosing the statements
 * in the well-known 'do { ... } while (constcond 0)' loop.  The 0 in the
 * controlling expression has type INT but should be allowed nevertheless.
 */
void
strict_bool_system_header_statement_macro(void)
{

	do {
		println("nothing");
	} while (/*CONSTCOND*/0);	/* expect: 333 */

# 27 "d_c99_bool_strict_syshdr.c" 3 4
	do {
		println("nothing");
	} while (/*CONSTCOND*/0);	/* ok */

# 32 "d_c99_bool_strict_syshdr.c"
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
 * Since that is more code to write and in exceptional situations more code
 * to execute, they will probably leave out the extra comparison, but both
 * ways are possible.
 *
 * In strict mode, there must be a way to call these function-like macros
 * portably, without triggering type errors, no matter whether they return
 * BOOL or INT.
 *
 * The expressions from this example cross the boundary between system header
 * and application code.  They need to carry the information that they are
 * half-BOOL, half-INT across the enclosing expressions.
 */
void
strict_bool_system_header_ctype(int c)
{
	static const unsigned short *ctype_table;


	/*
	 * The macro returns INT, which may be outside the range of a
	 * uint8_t variable, therefore it must not be assigned directly.
	 * All other combinations of type are safe from truncation.
	 */
	_Bool system_int_assigned_to_bool =
# 69 "d_c99_bool_strict_syshdr.c" 3 4
	    (int)((ctype_table + 1)[c] & 0x0040)	/* INT */
# 71 "d_c99_bool_strict_syshdr.c"
	    ;			/* expect: 107 */

	int system_bool_assigned_to_int =
# 75 "d_c99_bool_strict_syshdr.c" 3 4
	    (int)((ctype_table + 1)[c] & 0x0040) != 0	/* BOOL */
# 77 "d_c99_bool_strict_syshdr.c"
	    ;			/* expect: 107 */

	if (
# 81 "d_c99_bool_strict_syshdr.c" 3 4
	    (int)((ctype_table + 1)[c] & 0x0040)	/* INT */
# 83 "d_c99_bool_strict_syshdr.c"
	)			/*FIXME*//* expect: 333 */
	println("system macro returning INT");

	if (
# 88 "d_c99_bool_strict_syshdr.c" 3 4
	    ((ctype_table + 1)[c] & 0x0040) != 0	/* BOOL */
# 90 "d_c99_bool_strict_syshdr.c"
	)
	println("system macro returning BOOL");
}
