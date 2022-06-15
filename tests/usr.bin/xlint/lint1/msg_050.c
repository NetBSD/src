/*	$NetBSD: msg_050.c,v 1.4 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_050.c"

/* Test for message: a function is declared as an argument: %s [50] */

/* lint1-flags: -Stw */

typedef void (function)();

/* expect+1: warning: argument 'f' unused in function 'example' [231] */
void example(f)
    /* expect+1: warning: a function is declared as an argument: f [50] */
    function f;
{
}
