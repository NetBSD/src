/*	$NetBSD: decl_struct_member.c,v 1.1 2021/06/19 19:49:15 rillig Exp $	*/
# 3 "decl_struct_member.c"

/*
 * Before cgram.y 1.228 from 2021-06-19, lint ran into an assertion failure:
 *
 * "is_struct_or_union(dcs->d_type->t_tspec)" at cgram.y:846
 */

struct {
	char;			/* expect: syntax error 'unnamed member' */
};
