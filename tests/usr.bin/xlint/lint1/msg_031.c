/*	$NetBSD: msg_031.c,v 1.4 2021/03/16 23:39:41 rillig Exp $	*/
# 3 "msg_031.c"

// Test for message: incomplete structure or union %s: %s [31]

struct complete {
	int dummy;
};

struct incomplete;			/* expect: 233 */


struct complete complete_var;

struct incomplete incomplete_var;	/* expect: 31 */

/* XXX: the 'incomplete: <unnamed>' in the diagnostic looks strange */
void function(struct incomplete);	/* expect: incomplete: <unnamed> [31] */
