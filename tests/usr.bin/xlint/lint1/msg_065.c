/*	$NetBSD: msg_065.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_065.c"

// Test for message: '%s' has no named members [65]

struct ok {
	int member;
};

/* XXX: should generate a warning as well. */
struct empty {
};

struct only_unnamed_members {
	unsigned int :14;
	unsigned int :0;
};
/* expect-1: warning: 'struct only_unnamed_members' has no named members [65] */
