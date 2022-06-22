/*	$NetBSD: msg_182.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_182.c"

// Test for message: incompatible pointer types to '%s' and '%s' [182]

void *
return_discarding_volatile(volatile void *arg)
{
	/* expect+1: warning: incompatible pointer types to 'void' and 'volatile void' [182] */
	return arg;
}

void
init_discarding_volatile(volatile void *arg)
{
	/* expect+1: warning: incompatible pointer types to 'void' and 'volatile void' [182] */
	void *array[] = { arg };
}
