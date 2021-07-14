/*	$NetBSD: decl_struct_member.c,v 1.6 2021/07/14 20:39:13 rillig Exp $	*/
# 3 "decl_struct_member.c"

struct multi_attributes {
	__attribute__((deprecated))
	__attribute__((deprecated))
	__attribute__((deprecated))
	int deprecated;
};

struct cover_begin_type_noclass_declspecs {
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

struct cover_begin_type_noclass_declmods {
	const m1;
	const volatile m2;
};

/*
 * Before cgram.y 1.228 from 2021-06-19, lint ran into an assertion failure:
 *
 * "is_struct_or_union(dcs->d_type->t_tspec)" at cgram.y:846
 */
struct {
	char;			/* expect: syntax error 'unnamed member' */
};

/*
 * Before decl.c 1.188 from 2021-06-20, lint ran into a segmentation fault.
 */
struct {
	char a(_)0		/* expect: syntax error '0' */
}				/* expect: ';' after last */
/*
 * FIXME: adding a semicolon here triggers another assertion:
 *
 * assertion "t == NOTSPEC" failed in end_type at decl.c:774
 */
/* expect+1: cannot recover from previous errors */
