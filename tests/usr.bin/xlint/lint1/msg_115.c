/*	$NetBSD: msg_115.c,v 1.4 2021/01/31 11:12:07 rillig Exp $	*/
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
