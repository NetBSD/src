/*	$NetBSD: msg_152.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_152.c"

// Test for message: argument cannot have unknown size, arg #%d [152]

/* expect+1: warning: struct 'incomplete' never defined [233] */
struct incomplete;

/* expect+1: error: '<unnamed>' has incomplete type 'incomplete struct incomplete' [31] */
void callee(struct incomplete);

void
caller(void)
{
	/* expect+1: error: 'local_var' has incomplete type 'incomplete struct incomplete' [31] */
	struct incomplete local_var;
	/* expect+1: error: argument cannot have unknown size, arg #1 [152] */
	callee(local_var);
}
