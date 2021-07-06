/*	$NetBSD: gcc_attribute_type.c,v 1.1 2021/07/06 17:33:07 rillig Exp $	*/
# 3 "gcc_attribute_type.c"

/*
 * Tests for the GCC __attribute__ for types.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html
 */

struct __attribute__((__packed__)) packed_record {
	unsigned char len[2];
	unsigned long magic;
};

struct record_packed {
	unsigned char len[2];
	unsigned long magic;
} __attribute__((__packed__));

/* TODO: do not allow __attribute__ before 'struct' */
__attribute__((__packed__))
struct attribute_before_keyword {
	unsigned char len[2];
	unsigned long magic;
};

/* just to trigger _some_ error, to keep the .exp file */
/* expect+1: error: syntax error 'syntax_error' [249] */
__attribute__((syntax_error));
