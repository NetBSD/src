/*	$NetBSD: msg_128.c,v 1.3 2021/03/16 23:12:30 rillig Exp $	*/
# 3 "msg_128.c"

// Test for message: operands have incompatible pointer types, op %s (%s != %s) [128]

void
conversion_to_unconst(const char *cstr)
{
	char *str;
	str = cstr;		/* expect: 128 */
}
