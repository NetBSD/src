/*	$NetBSD: gcc_attribute_enum.c,v 1.5 2022/06/17 18:54:53 rillig Exp $	*/
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

enum __attribute__(()) tag;

enum __attribute__(()) tag_with_declaration {
	TAG_WITH_DECL
} __attribute__(());

enum __attribute__(()) {
	ONLY_DECL
} __attribute__(());

/*
 * Attributes in enumerator.
 *
 * See GCC, c-parser.c, function c_parser_enum_specifier.
 */

enum without_initializer {
	NO_INIT_FIRST __attribute__(()),
	NO_INIT_LAST __attribute__(())
};

enum with_initializer {
	INIT_FIRST __attribute__(()) = 1,
	INIT_LAST __attribute__(()) = 2,
	/* expect+1: error: syntax error '__attribute__' [249] */
	INIT_WRONG = 3 __attribute__(()),
};

enum tag {
	TAG
};
