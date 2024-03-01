/*	$NetBSD: msg_355.c,v 1.2 2024/03/01 17:22:55 rillig Exp $	*/
# 3 "msg_355.c"

// Test for message: '_Static_assert' without message requires C23 or later [355]
//
// See also:
//	c23.c

/* lint1-extra-flags: -Ac11 -X 351 */

_Static_assert(1 > 0, "message");
/* expect+1: error: '_Static_assert' without message requires C23 or later [355] */
_Static_assert(1 > 0);
