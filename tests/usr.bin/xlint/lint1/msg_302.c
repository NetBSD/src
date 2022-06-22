/*	$NetBSD: msg_302.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_302.c"

// Test for message: '%s' returns pointer to automatic object [302]

void *
return_arg(int arg)
{
	/* expect+1: warning: 'return_arg' returns pointer to automatic object [302] */
	return &arg;
}

void *
return_local(void)
{
	int local = 3;
	/* expect+1: warning: 'return_local' returns pointer to automatic object [302] */
	return &local;
}

void *
return_local_array(_Bool cond)
{
	int local[5];
	int *p = local;

	/* XXX: lint doesn't track this indirection, but Clang-tidy does. */
	if (cond)
		return p;

	/* expect+1: warning: 'return_local_array' returns pointer to automatic object [302] */
	return local + 5;
}

void *
return_static(void)
{
	static int long_lived = 3;
	return &long_lived;
}
