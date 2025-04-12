/*	$NetBSD: msg_111.c,v 1.4 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_111.c"

/* Test for message: unacceptable operand of '%s' [111] */

/* lint1-flags: -tw -aa -chapbrzgF */

struct s {
	int member;
};

void
invalid_member_access()
{
	/* expect+2: warning: left operand of '.' must be struct or union, not 'function() returning void' [103] */
	/* expect+1: error: unacceptable operand of '.' [111] */
	return invalid_member_access.member;
}
