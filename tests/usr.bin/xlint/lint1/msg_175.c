/*	$NetBSD: msg_175.c,v 1.6 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_175.c"

// Test for message: initialization of incomplete type '%s' [175]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;

/* expect+1: error: initialization of incomplete type 'incomplete struct incomplete' [175] */
struct incomplete incomplete = {
	"invalid"
};
/* expect-1: error: 'incomplete' has incomplete type 'incomplete struct incomplete' [31] */
