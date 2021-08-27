/*	$NetBSD: msg_179.c,v 1.3 2021/08/27 19:50:44 rillig Exp $	*/
# 3 "msg_179.c"

// Test for message: cannot initialize struct/union with no named member [179]
/* This message is not used. */

struct {
	unsigned int :0;
} no_named_member = {
	/* no named member, therefore no initializer expression */
};

struct {
	/* no members */
} empty = {
	/* no initializer expressions */
};

struct {
	unsigned int: 0;
} no_named_member_init = {
	/* expect+1: error: too many struct/union initializers [172] */
	3,
};
