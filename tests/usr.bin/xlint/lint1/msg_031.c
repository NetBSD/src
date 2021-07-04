/*	$NetBSD: msg_031.c,v 1.5 2021/07/04 13:31:10 rillig Exp $	*/
# 3 "msg_031.c"

// Test for message: argument '%s' has type '%s' [31]

struct complete {
	int dummy;
};

struct incomplete;			/* expect: 233 */


struct complete complete_var;

struct incomplete incomplete_var;	/* expect: 31 */


/* expect+1: error: argument '<unnamed>' has type 'incomplete struct incomplete' [31] */
void function(struct incomplete);
