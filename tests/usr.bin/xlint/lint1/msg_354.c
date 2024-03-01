/*	$NetBSD: msg_354.c,v 1.2 2024/03/01 17:22:55 rillig Exp $	*/
# 3 "msg_354.c"

// Test for message: '_Static_assert' requires C11 or later [354]
//
// See also:
//	c23.c

/* lint1-flags: -Sw -X 351 */

/* expect+1: error: '_Static_assert' requires C11 or later [354] */
_Static_assert(1 > 0, "message");
/* expect+1: error: '_Static_assert' without message requires C23 or later [355] */
_Static_assert(1 > 0);
