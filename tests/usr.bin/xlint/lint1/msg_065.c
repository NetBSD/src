/*	$NetBSD: msg_065.c,v 1.3 2021/07/13 21:50:05 rillig Exp $	*/
# 3 "msg_065.c"

// Test for message: %s has no named members [65]

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
/* expect-1: warning: structure has no named members [65] */
