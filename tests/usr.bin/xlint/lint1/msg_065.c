/*	$NetBSD: msg_065.c,v 1.5 2023/06/30 15:19:09 rillig Exp $	*/
# 3 "msg_065.c"

// Test for message: '%s' has no named members [65]

struct ok {
	int member;
};

/* Don't warn about completely empty structs, which are a GCC extension. */
struct empty {
};

struct only_unnamed_members {
	unsigned int :14;
	unsigned int :0;
};
/* expect-1: warning: 'struct only_unnamed_members' has no named members [65] */
