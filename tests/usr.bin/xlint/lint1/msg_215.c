/*	$NetBSD: msg_215.c,v 1.6 2021/06/30 14:15:39 rillig Exp $	*/
# 3 "msg_215.c"

// Test for message: function implicitly declared to return int [215]

/*
 * In traditional C and C90, it was possible to implicitly declare a function
 * by just calling it, without defining a prototype first.  Such a function
 * would then be defined as taking unspecified parameters and returning int.
 */

struct str {
	int dummy;
};

/* ARGSUSED */
void
test(struct str str)
{
	/* expect+1: error: function implicitly declared to return int [215] */
	name();

	/* expect+2: error: 'parenthesized' undefined [99] */
	/* expect+1: error: illegal function (type int) [149] */
	(parenthesized)();

	/* expect+2: error: type 'struct str' does not have member 'member' [101] */
	/* expect+1: error: illegal function (type int) [149] */
	str.member();
}
