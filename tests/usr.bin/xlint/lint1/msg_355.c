/*	$NetBSD: msg_355.c,v 1.1 2023/08/03 18:48:42 rillig Exp $	*/
# 3 "msg_355.c"

// Test for message 355: '_Static_assert' without message requires C23 or later [355]
//
// See also:
//	c23.c

/* lint1-extra-flags: -Ac11 -X 351 */

_Static_assert(1 > 0, "message");
/* expect+1: error: '_Static_assert' without message requires C23 or later [355] */
_Static_assert(1 > 0);
