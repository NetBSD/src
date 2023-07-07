/*	$NetBSD: msg_302.c,v 1.8 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_302.c"

// Test for message: '%s' returns pointer to automatic object [302]

/* lint1-extra-flags: -X 351 */

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
	case 6:
		/* expect+1: warning: 'return_local_array' returns pointer to automatic object [302] */
		return (local);
	case 7:
		/* C99 6.5.2.5p6 */
		/* expect+1: warning: 'return_local_array' returns pointer to automatic object [302] */
		return (char[]){"local string"};
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
