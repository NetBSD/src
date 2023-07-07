/*	$NetBSD: msg_050.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_050.c"

/* Test for message: argument '%s' has function type, should be pointer [50] */

/* lint1-flags: -tw */

typedef void (function)();

/* expect+1: warning: argument 'f' unused in function 'example' [231] */
void example(f)
    /* expect+1: warning: argument 'f' has function type, should be pointer [50] */
    function f;
{
}
