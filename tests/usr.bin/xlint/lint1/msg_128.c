/*	$NetBSD: msg_128.c,v 1.6 2023/07/07 06:03:31 rillig Exp $	*/
# 3 "msg_128.c"

// Test for message: operands of '%s' have incompatible pointer types to '%s' and '%s' [128]

void
conversion_to_unconst(const char *cstr)
{
	char *str;
	/* expect+2: warning: 'str' set but not used in function 'conversion_to_unconst' [191] */
	/* expect+1: warning: operands of '=' have incompatible pointer types to 'char' and 'const char' [128] */
	str = cstr;
}
