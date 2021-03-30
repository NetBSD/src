/*	$NetBSD: msg_175.c,v 1.4 2021/03/30 15:07:53 rillig Exp $	*/
# 3 "msg_175.c"

// Test for message: initialization of incomplete type '%s' [175]

struct incomplete;			/* expect: 233 */

struct incomplete incomplete = {	/* expect: 175 */
	"invalid"
};					/* expect: 31 */
