/*	$NetBSD: msg_115.c,v 1.5 2021/07/31 09:14:47 rillig Exp $	*/
# 3 "msg_115.c"

// Test for message: %soperand of '%s' must be modifiable lvalue [115]

void
example(const int *const_ptr)
{

	*const_ptr = 3;		/* expect: 115 */
	*const_ptr += 1;	/* expect: 115 */
	*const_ptr -= 4;	/* expect: 115 */
	*const_ptr *= 1;	/* expect: 115 */
	*const_ptr /= 5;	/* expect: 115 */
	*const_ptr %= 9;	/* expect: 115 */
	(*const_ptr)++;		/* expect: 115 */
}

void
initialize_const_struct_member(void)
{
	struct S {
		int const member;
	};

	/* FIXME: In an initialization, const members can be assigned. */
	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	struct S s1 = (struct S) { 12345 };
	if (s1.member != 0)
		return;

	struct S s2 = { 12345 };
	if (s2.member != 0)
		return;
}
