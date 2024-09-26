/*	$NetBSD: msg_231.c,v 1.7 2024/09/26 21:31:09 rillig Exp $	*/
# 3 "msg_231.c"

// Test for message: parameter '%s' unused in function '%s' [231]
//
// See also:
//	msg_192		for unused local variables

/* lint1-extra-flags: -X 351 */

void
example(
	/* expect+1: warning: parameter 'param_scalar' unused in function 'example' [231] */
	int param_scalar,
	/* expect+1: warning: parameter 'param_ptr' unused in function 'example' [231] */
	char *param_ptr,
	/* expect+1: warning: parameter 'param_arr' unused in function 'example' [231] */
	char param_arr[5],
	/* expect+1: warning: parameter 'param_func' unused in function 'example' [231] */
	void (*param_func)(int, double),
	/* expect+1: warning: parameter 'param_signal' unused in function 'example' [231] */
	void (*param_signal(int sig, void (*handler)(int)))(int)
)
{
}
