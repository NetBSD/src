/*	$NetBSD: msg_138.c,v 1.5 2022/07/01 20:53:13 rillig Exp $	*/
# 3 "msg_138.c"

// Test for message: unknown operand size, op '%s' [138]

/* lint1-extra-flags: -z */

struct incomplete;

/*
 * This code doesn't make sense at all, at least not in C99.
 */
/* ARGSUSED */
void
function(_Bool cond, struct incomplete *i1, struct incomplete *i2)
{
	/* expect+2: error: 'local' has incomplete type 'incomplete struct incomplete' [31] */
	/* expect+1: error: cannot initialize 'incomplete struct incomplete' from 'pointer to incomplete struct incomplete' [185] */
	struct incomplete local = i1;

	/* expect+1: error: unknown operand size, op '=' [138] */
	*i1 = *i2;

	/* expect+1: error: unknown operand size, op ':' [138] */
	return cond ? *i1 : *i2;
}

/* ARGSUSED */
struct incomplete
return_incomplete(struct incomplete *ptr)
/* expect+1: error: cannot return incomplete type [67] */
{
	/* expect+1: error: cannot return incomplete type [212] */
	return *ptr;
}
