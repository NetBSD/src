/*	$NetBSD: msg_115.c,v 1.10 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_115.c"

// Test for message: %soperand of '%s' must be modifiable lvalue [115]

void
example(const int *const_ptr)
{

	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	*const_ptr = 3;
	/* expect+1: warning: left operand of '+=' must be modifiable lvalue [115] */
	*const_ptr += 1;
	/* expect+1: warning: left operand of '-=' must be modifiable lvalue [115] */
	*const_ptr -= 4;
	/* expect+1: warning: left operand of '*=' must be modifiable lvalue [115] */
	*const_ptr *= 1;
	/* expect+1: warning: left operand of '/=' must be modifiable lvalue [115] */
	*const_ptr /= 5;
	/* expect+1: warning: left operand of '%=' must be modifiable lvalue [115] */
	*const_ptr %= 9;
	/* expect+1: warning: operand of 'x++' must be modifiable lvalue [115] */
	(*const_ptr)++;

	/* In the next example, the left operand is not an lvalue at all. */
	/* expect+1: error: left operand of '=' must be lvalue [114] */
	(const_ptr + 3) = const_ptr;
}

typedef struct {
	int const member;
} const_member;

void take_const_member(const_member);

/*
 * Before init.c 1.208 from 2021-08-14 and decl.c 1.221 from 2021-08-10,
 * lint issued a wrong "warning: left operand of '%s' must be modifiable
 * lvalue", even in cases where the left operand was being initialized
 * instead of overwritten.
 *
 * See initialization_expr_using_op, typeok_assign, has_constant_member.
 * See C99 6.2.5p25.
 */
const_member
initialize_const_struct_member(void)
{
	/* In a simple initialization, const members can be assigned. */
	const_member cm1 = (const_member) { 12345 };

	if (cm1.member != 0)
		/* In a function call, const members can be assigned. */
		take_const_member(cm1);

	struct {
		const_member member;
	} cm2 = {
	    /* In a nested initialization, const members can be assigned. */
	    cm1,
	};
	if (cm2.member.member != 0) {
	}

	/* In a return statement, const members can be assigned. */
	return cm1;
}
