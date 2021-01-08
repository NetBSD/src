/*	$NetBSD: msg_050.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_050.c"

/* Test for message: a function is declared as an argument: %s [50] */

/* lint1-flags: -Stw */

typedef void (function)();

void example(f)
    function f;
{
}
