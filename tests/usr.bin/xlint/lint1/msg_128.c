/*	$NetBSD: msg_128.c,v 1.8 2024/01/28 08:17:27 rillig Exp $	*/
# 3 "msg_128.c"

// Test for message: operands of '%s' have incompatible pointer types to '%s' and '%s' [128]

/* lint1-extra-flags: -X 351 */

void
conversion_to_unconst(const char *cstr)
{
	char *str;
	/* expect+2: warning: operands of '=' have incompatible pointer types to 'char' and 'const char' [128] */
	/* expect+1: warning: 'str' set but not used in function 'conversion_to_unconst' [191] */
	str = cstr;
}
