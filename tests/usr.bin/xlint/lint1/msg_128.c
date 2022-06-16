/*	$NetBSD: msg_128.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_128.c"

// Test for message: operands have incompatible pointer types, op %s (%s != %s) [128]

void
conversion_to_unconst(const char *cstr)
{
	char *str;
	/* expect+1: warning: operands have incompatible pointer types, op = (char != const char) [128] */
	str = cstr;
}
