/*	$NetBSD: msg_231.c,v 1.9 2024/09/28 15:51:40 rillig Exp $	*/
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
	void (*param_signal(int sig, void (*handler)(int)))(int),

	int param_scalar_attr __attribute__((__unused__)),
	char *param_ptr_attr __attribute__((__unused__)),
	char param_arr_attr[5] __attribute__((__unused__)),
	void (*param_func_attr)(int, double) __attribute__((__unused__)),
	void (*param_signal_attr(int sig, void (*handler)(int)))(int) __attribute__((__unused__))
)
{
}
