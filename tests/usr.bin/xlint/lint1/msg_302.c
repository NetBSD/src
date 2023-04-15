/*	$NetBSD: msg_302.c,v 1.5 2023/04/15 10:53:59 rillig Exp $	*/
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
return_local_array(int x)
{
	int local[5], *indirect = local;

	switch (x) {
	case 0:
		/* expect+1: warning: 'return_local_array' returns pointer to automatic object [302] */
		return local;
	case 1:
		/* expect+1: warning: 'return_local_array' returns pointer to automatic object [302] */
		return &local[3];
	case 2:
		/* expect+1: warning: 'return_local_array' returns pointer to automatic object [302] */
		return 5 + local;
	case 3:
		/* expect+1: warning: 'return_local_array' returns pointer to automatic object [302] */
		return local + 5;
	case 4:
		/* XXX: lint only checks '+' but not '-'. */
		return local - -3;
	case 5:
		/* XXX: lint doesn't track this indirection, but Clang-tidy does. */
		return indirect;
	default:
		return "OK";
	}
}

void *
return_static(void)
{
	static int long_lived = 3;
	return &long_lived;
}
