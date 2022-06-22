/*	$NetBSD: msg_017.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_017.c"

// Test for message: null dimension [17]

/* expect+1: warning: empty array declaration for 'empty_array_var' [190] */
int empty_array_var[0];

typedef int empty_array_type[0];
/* expect+1: warning: empty array declaration for 'typedef_var' [190] */
empty_array_type typedef_var;

struct s {
	int empty_array_member[0];
	/* expect+1: error: null dimension [17] */
	int empty_multi_array_member[0][0];
	int other_member;
};
