/*	$NetBSD: msg_111.c,v 1.3.4.1 2025/08/02 05:58:16 perseant Exp $	*/
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
