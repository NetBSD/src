/*	$NetBSD: msg_050.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_050.c"

/* Test for message: a function is declared as an argument: %s [50] */

/* lint1-flags: -Stw */

typedef void (function)();

void example(f)			/* expect: 231 */
    function f;			/* expect: 50 */
{
}
