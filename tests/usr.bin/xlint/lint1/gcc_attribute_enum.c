/*	$NetBSD: gcc_attribute_enum.c,v 1.2 2021/07/25 18:34:44 rillig Exp $	*/
# 3 "gcc_attribute_enum.c"

/*
 * Tests for the GCC __attribute__ for enumerators.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Enumerator-Attributes.html
 */

/*
 * Attributes in enum-specifier.
 *
 * See GCC, c-parser.c, function c_parser_enum_specifier.
 */

/* expect+1: syntax error '__attribute__' [249] */
enum __attribute__(()) tag;

/* expect+2: syntax error '__attribute__' [249] */
/* expect+1: syntax error '{' [249] */
enum __attribute__(()) tag_with_declaration {
	TAG_WITH_DECL
} __attribute__(());
/* expect-1: syntax error ';' [249] */

/* expect+1: syntax error '{' [249] */
enum __attribute__(()) {
	ONLY_DECL
} __attribute__(());
/* expect-1: syntax error ';' [249] */
/* expect-2: error: cannot recover from previous errors [224] */

/*
 * Attributes in enumerator.
 *
 * See GCC, c-parser.c, function c_parser_enum_specifier.
 */

enum {
	NO_INIT_FIRST __attribute__(()),
	NO_INIT__LAST __attribute__(())
};

enum {
	INIT_FIRST __attribute__(()) = 1,
	INIT__LAST __attribute__(()) = 2
};
