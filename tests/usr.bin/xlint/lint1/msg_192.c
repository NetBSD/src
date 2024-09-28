/*	$NetBSD: msg_192.c,v 1.12 2024/09/28 15:51:40 rillig Exp $	*/
# 3 "msg_192.c"

// Test for message: '%s' unused in function '%s' [192]
//
// See also:
//	msg_231		for unused parameters

/* lint1-extra-flags: -X 351 */

void
example(void)
{
	/* expect+1: warning: 'local_scalar' unused in function 'example' [192] */
	int local_scalar;
	/* expect+1: warning: 'local_ptr' unused in function 'example' [192] */
	char *local_ptr;
	/* expect+1: warning: 'local_arr' unused in function 'example' [192] */
	char local_arr[5];
	/* expect+1: warning: 'local_func' unused in function 'example' [192] */
	void (*local_func)(int, double);
	typedef void (*handler)(int);
	/* expect+1: warning: 'local_signal' unused in function 'example' [192] */
	handler (*local_signal)(int, handler);

	int local_scalar_attr __attribute__((__unused__));
	char *local_ptr_attr __attribute__((__unused__));
	char local_arr_attr[5] __attribute__((__unused__));
	void (*local_func_attr)(int, double) __attribute__((__unused__));
	void (*(*local_signal_attr)(int sig, void (*handler)(int)))(int) __attribute__((__unused__));
}


void assertion_failed(const char *, int, const char *, const char *);

/*
 * The symbol '__func__' only occurs in an unreachable branch.  It is
 * nevertheless marked as used.
 */
void
assert_true(void)
{
	sizeof(char) == 1
	    ? (void)0
	    : assertion_failed("file", 26, __func__, "sizeof(char) == 1");
}

void
assert_false(void)
{
	sizeof(char) == 0
	    ? (void)0
	    : assertion_failed("file", 34, __func__, "sizeof(char) == 0");
}

void
assert_unknown(_Bool cond)
{
	cond
	    ? (void)0
	    : assertion_failed("file", 42, __func__, "cond");
}
