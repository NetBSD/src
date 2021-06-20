/*	$NetBSD: decl_struct_member.c,v 1.2 2021/06/20 11:24:32 rillig Exp $	*/
# 3 "decl_struct_member.c"

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
 * assertion "t == NOTSPEC" failed in deftyp at decl.c:774
 */
/* expect+1: cannot recover from previous errors */
