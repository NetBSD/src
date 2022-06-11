/*	$NetBSD: msg_314.c,v 1.3 2022/06/11 11:20:40 rillig Exp $	*/
# 3 "msg_314.c"

// Test for message: %s is not a structure or a union [314]
/* This message is not used. */

/*
 * Added in err.c 1.20 from 2002-10-21 when adding support for C99's
 * designated initializers, but never actually used.
 *
 * Instead of that message, trigger some related messages.
 */
void
function(int x)
{
	/* expect+1: error: type 'int' does not have member 'member' [101] */
	x->member++;
	/* expect+1: error: type 'int' does not have member 'member' [101] */
	x.member++;
}
