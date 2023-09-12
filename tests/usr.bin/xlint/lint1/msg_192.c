/*	$NetBSD: msg_192.c,v 1.8 2023/09/12 07:23:27 rillig Exp $	*/
# 3 "msg_192.c"

// Test for message: '%s' unused in function '%s' [192]

/* lint1-extra-flags: -X 351 */

void
/* expect+1: warning: parameter 'param' unused in function 'example' [231] */
example(int param)
{
	/* expect+1: warning: 'local' unused in function 'example' [192] */
	int local;
}


void assertion_failed(const char *);

/*
 * The symbol '__func__' only occurs in an unreachable branch.  It is
 * nevertheless marked as used.
 */
void
assert_true(void)
{
	sizeof(char) == 1
	    ? (void)0
	    : assertion_failed(__func__);
}

void
assert_false(void)
{
	sizeof(char) == 0
	    ? (void)0
	    : assertion_failed(__func__);
}

void
assert_unknown(_Bool cond)
{
	cond
	    ? (void)0
	    : assertion_failed(__func__);
}
