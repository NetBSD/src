/*	$NetBSD: msg_215.c,v 1.9 2021/06/30 14:42:13 rillig Exp $	*/
# 3 "msg_215.c"

// Test for message: function '%s' implicitly declared to return int [215]

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
	/* expect+1: warning: function 'name' implicitly declared to return int [215] */
	name();

	/* expect+2: error: 'parenthesized' undefined [99] */
	/* expect+1: error: illegal function (type int) [149] */
	(parenthesized)();

	/* expect+2: error: type 'struct str' does not have member 'member' [101] */
	/* expect+1: error: illegal function (type int) [149] */
	str.member();

	/* https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html */
	__builtin_whatever(123, "string");
	__atomic_whatever(123, "string");
}
