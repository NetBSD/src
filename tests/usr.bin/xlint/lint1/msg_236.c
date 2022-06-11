/*	$NetBSD: msg_236.c,v 1.4 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "msg_236.c"

// Test for message: static function '%s' unused [236]

void
external_function(void)
{
}

/* expect+2: warning: static function 'static_function' unused [236] */
static void
static_function(void)
{
}
