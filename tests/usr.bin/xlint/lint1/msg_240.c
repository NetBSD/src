/*	$NetBSD: msg_240.c,v 1.5 2023/07/07 06:03:31 rillig Exp $	*/
# 3 "msg_240.c"

// Test for message: assignment of different structures (%s != %s) [240]
// This message is not used.

struct s_arg {
	int member;
};

struct s_local {
	int member;
};

struct s_return {
	int member;
};

union u_arg {
	int member;
};

/* expect+2: warning: argument 's_arg' unused in function 'return_other_struct' [231] */
struct s_return
return_other_struct(struct s_arg s_arg)
{
	/* XXX: No warning? */
	return s_arg;
}

/* expect+2: warning: argument 's_arg' unused in function 'assign_other_struct' [231] */
void
assign_other_struct(struct s_arg s_arg)
{
	/* expect+1: warning: 's_local' unused in function 'assign_other_struct' [192] */
	static struct s_local s_local;
	/* XXX: No warning? */
	s_local = s_arg;
}

/* expect+2: warning: argument 'u_arg' unused in function 'return_other_union' [231] */
struct s_return
return_other_union(union u_arg u_arg)
{
	/* XXX: No warning? */
	return u_arg;
}
