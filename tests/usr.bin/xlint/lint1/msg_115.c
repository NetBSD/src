/*	$NetBSD: msg_115.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_115.c"

// Test for message: %soperand of '%s' must be modifiable lvalue [115]

void
example(const int *const_ptr)
{

	*const_ptr = 3;
	*const_ptr += 1;
	*const_ptr -= 4;
	*const_ptr *= 1;
	*const_ptr /= 5;
	*const_ptr %= 9;
}
