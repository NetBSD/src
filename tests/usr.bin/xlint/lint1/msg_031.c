/*	$NetBSD: msg_031.c,v 1.6 2021/07/13 22:01:34 rillig Exp $	*/
# 3 "msg_031.c"

// Test for message: '%s' has incomplete type '%s' [31]

struct complete {
	int dummy;
};

struct incomplete;			/* expect: 233 */


struct complete complete_var;

/* expect+1: 'incomplete_var' has incomplete type 'incomplete struct incomplete' */
struct incomplete incomplete_var;


/* expect+1: '<unnamed>' has incomplete type 'incomplete struct incomplete' [31] */
void function(struct incomplete);
