/*	$NetBSD: msg_031.c,v 1.8 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_031.c"

// Test for message: '%s' has incomplete type '%s' [31]

struct complete {
	int dummy;
};

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;


struct complete complete_var;

/* expect+1: error: 'incomplete_var' has incomplete type 'incomplete struct incomplete' [31] */
struct incomplete incomplete_var;


/* expect+1: error: '<unnamed>' has incomplete type 'incomplete struct incomplete' [31] */
void function(struct incomplete);
