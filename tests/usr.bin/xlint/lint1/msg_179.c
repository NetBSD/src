/*	$NetBSD: msg_179.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_179.c"

// Test for message: cannot initialize struct/union with no named member [179]

/* lint1-extra-flags: -X 351 */

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
