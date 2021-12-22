/*	$NetBSD: msg_179.c,v 1.4 2021/12/22 00:45:53 rillig Exp $	*/
# 3 "msg_179.c"

// Test for message: cannot initialize struct/union with no named member [179]

struct {
	unsigned int :0;
} no_named_member = {
	/* expect-1: error: cannot initialize struct/union with no named member [179] */
	/* no named member, therefore no initializer expression */
};

struct {
	/* no members */
} empty = {
	/* expect-1: error: cannot initialize struct/union with no named member [179] */
	/* no initializer expressions */
};

struct {
	unsigned int: 0;
} no_named_member_init = {
	/* expect-1: error: cannot initialize struct/union with no named member [179] */
	3,
};
