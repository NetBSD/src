/*	$NetBSD: msg_240.c,v 1.8 2023/08/02 18:51:25 rillig Exp $	*/
# 3 "msg_240.c"

// Test for message: assignment of different structures (%s != %s) [240]
// This message is not used.

/* lint1-extra-flags: -X 351 */

struct s_param {
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

/* expect+2: warning: parameter 's_param' unused in function 'return_other_struct' [231] */
struct s_return
return_other_struct(struct s_param s_param)
{
	/* XXX: No warning? */
	return s_param;
}

/* expect+2: warning: parameter 's_param' unused in function 'assign_other_struct' [231] */
void
assign_other_struct(struct s_param s_param)
{
	/* expect+1: warning: 's_local' unused in function 'assign_other_struct' [192] */
	static struct s_local s_local;
	/* XXX: No warning? */
	s_local = s_param;
}

/* expect+2: warning: parameter 'u_arg' unused in function 'return_other_union' [231] */
struct s_return
return_other_union(union u_arg u_arg)
{
	/* XXX: No warning? */
	return u_arg;
}
