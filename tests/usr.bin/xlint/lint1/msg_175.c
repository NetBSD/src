/*	$NetBSD: msg_175.c,v 1.3 2021/02/22 15:09:50 rillig Exp $	*/
# 3 "msg_175.c"

// Test for message: initialization of an incomplete type [175]

struct incomplete;			/* expect: 233 */

struct incomplete incomplete = {	/* expect: 175 */
	"invalid"
};					/* expect: 31 */
