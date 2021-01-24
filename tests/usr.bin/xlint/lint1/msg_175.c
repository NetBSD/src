/*	$NetBSD: msg_175.c,v 1.2 2021/01/24 16:12:45 rillig Exp $	*/
# 3 "msg_175.c"

// Test for message: initialisation of an incomplete type [175]

struct incomplete;			/* expect: 233 */

struct incomplete incomplete = {	/* expect: 175 */
	"invalid"
};					/* expect: 31 */
