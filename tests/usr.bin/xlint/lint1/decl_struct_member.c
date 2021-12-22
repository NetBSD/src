/*	$NetBSD: decl_struct_member.c,v 1.13 2021/12/22 14:49:11 rillig Exp $	*/
# 3 "decl_struct_member.c"

struct multi_attributes {
	__attribute__((deprecated))
	__attribute__((deprecated))
	__attribute__((deprecated))
	int deprecated;
};

struct cover_begin_type_specifier_qualifier_list {
	int m1;
	__attribute__((deprecated)) int m2;
	const int m3;
	int const m4;
	int const long m5;
	int __attribute__((deprecated)) m6;
};

typedef int number;

struct cover_begin_type_typespec {
	int m1;
	number m2;
};

struct cover_begin_type_qualifier_list {
	const m1;
	const volatile m2;
};

/* cover struct_or_union_specifier: struct_or_union error */
/* expect+1: error: syntax error 'goto' [249] */
struct goto {
	/* expect+1: error: illegal type combination [4] */
	int member;
	/* expect+1: error: syntax error '}' [249] */
};
/* expect-1: warning: empty declaration [0] */

/*
 * Before cgram.y 1.228 from 2021-06-19, lint ran into an assertion failure:
 *
 * "is_struct_or_union(dcs->d_type->t_tspec)" at cgram.y:846
 */
struct {
	char;			/* expect: syntax error 'unnamed member' */
};

struct cover_notype_struct_declarators {
	const a, b;
};

struct cover_notype_struct_declarator_bit_field {
	const a:3, :0, b:4;
	const:0;
};

/*
 * An array of bit-fields sounds like a strange idea since a bit-field member
 * is not addressable, while an array needs to be addressable.  Due to this
 * contradiction, this combination may have gone without mention in the C
 * standards.
 *
 * GCC 10.3.0 complains that the bit-field has invalid type.
 *
 * Clang 12.0.1 complains that the bit-field has non-integral type 'unsigned
 * int [8]'.
 */
struct array_of_bit_fields {
	/* expect+1: warning: illegal bit-field type 'array[8] of unsigned int' [35] */
	unsigned int bits[8]: 1;
};

/*
 * Before decl.c 1.188 from 2021-06-20, lint ran into a segmentation fault.
 */
struct {
	char a(_)0		/* expect: syntax error '0' */

/*
 * Before cgram.y 1.328 from 2021-07-15, lint ran into an assertion failure
 * at the closing semicolon:
 *
 * assertion "t == NOTSPEC" failed in end_type at decl.c:774
 */
};
/* expect+1: cannot recover from previous errors */
