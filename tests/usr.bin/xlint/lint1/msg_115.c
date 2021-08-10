/*	$NetBSD: msg_115.c,v 1.7 2021/08/10 20:43:13 rillig Exp $	*/
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

typedef struct {
	int const member;
} const_member;

void take_const_member(const_member);

/* see typeok_assign, has_constant_member */
const_member
initialize_const_struct_member(void)
{
	/* FIXME: In an initialization, const members can be assigned. */
	/* expect+1: warning: left operand of '=' must be modifiable lvalue [115] */
	const_member cm1 = (const_member) { 12345 };
	if (cm1.member != 0)
		/* In a function call, const members can be assigned. */
		take_const_member(cm1);

	struct {
		const_member member;
	} cm2 = {
	    /* In an initialization, const members can be assigned. */
	    cm1,
	};
	if (cm2.member.member != 0) {
	}

	/* In a return statement, const members can be assigned. */
	return cm1;
}
