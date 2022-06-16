/*	$NetBSD: msg_111.c,v 1.3 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_111.c"

/* Test for message: unacceptable operand of '%s' [111] */

/* lint1-flags: -tw -aa -chapbrzgF */

struct s {
	int member;
};

void
illegal_member_access()
{
	/* expect+2: warning: left operand of '.' must be struct or union, not 'function() returning void' [103] */
	/* expect+1: error: unacceptable operand of '.' [111] */
	return illegal_member_access.member;
}
