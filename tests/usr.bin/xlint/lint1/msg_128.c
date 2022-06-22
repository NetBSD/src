/*	$NetBSD: msg_128.c,v 1.5 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_128.c"

// Test for message: operands of '%s' have incompatible pointer types to '%s' and '%s' [128]

void
conversion_to_unconst(const char *cstr)
{
	char *str;
	/* expect+1: warning: operands of '=' have incompatible pointer types to 'char' and 'const char' [128] */
	str = cstr;
}
