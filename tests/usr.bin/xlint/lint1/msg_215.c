/*	$NetBSD: msg_215.c,v 1.4 2021/06/30 13:50:15 rillig Exp $	*/
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

	/* FIXME: "type 'int'" sounds wrong. */
	/* expect+2: error: type 'int' does not have member 'member' [101] */
	/* expect+1: error: illegal function (type int) [149] */
	str.member();
}
