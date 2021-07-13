/*	$NetBSD: msg_067.c,v 1.3 2021/07/13 21:50:05 rillig Exp $	*/
# 3 "msg_067.c"

// Test for message: cannot return incomplete type [67]

/* expect+1: warning: struct incomplete never defined [233] */
struct incomplete;

struct incomplete function_declaration(void);

struct incomplete
function_definition(void)
/* expect+1: error: cannot return incomplete type [67] */
{
	/* FIXME: 'r' is not an argument. */
	/* expect+1: error: argument 'r' has type 'incomplete struct incomplete' [31] */
	struct incomplete r;

	/* expect+1: error: cannot return incomplete type [212] */
	return r;
}
