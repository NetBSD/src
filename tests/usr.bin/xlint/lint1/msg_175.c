/*	$NetBSD: msg_175.c,v 1.5 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_175.c"

// Test for message: initialization of incomplete type '%s' [175]

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;

/* expect+1: error: initialization of incomplete type 'incomplete struct incomplete' [175] */
struct incomplete incomplete = {
	"invalid"
};
/* expect-1: error: 'incomplete' has incomplete type 'incomplete struct incomplete' [31] */
