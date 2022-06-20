/*	$NetBSD: msg_050.c,v 1.5 2022/06/20 21:13:36 rillig Exp $	*/
# 3 "msg_050.c"

/* Test for message: argument '%s' has function type, should be pointer [50] */

/* lint1-flags: -Stw */

typedef void (function)();

/* expect+1: warning: argument 'f' unused in function 'example' [231] */
void example(f)
    /* expect+1: warning: argument 'f' has function type, should be pointer [50] */
    function f;
{
}
