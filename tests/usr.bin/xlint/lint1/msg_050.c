/*	$NetBSD: msg_050.c,v 1.7 2023/07/09 11:18:55 rillig Exp $	*/
# 3 "msg_050.c"

/* Test for message: parameter '%s' has function type, should be pointer [50] */

/* lint1-flags: -tw */

typedef void (function)();

/* expect+1: warning: parameter 'f' unused in function 'example' [231] */
void example(f)
    /* expect+1: warning: parameter 'f' has function type, should be pointer [50] */
    function f;
{
}
