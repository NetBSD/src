/*	$NetBSD: msg_182.c,v 1.3 2021/08/16 18:51:58 rillig Exp $	*/
# 3 "msg_182.c"

// Test for message: incompatible pointer types (%s != %s) [182]

void *
return_discarding_volatile(volatile void *arg)
{
	/* expect+1: warning: incompatible pointer types (void != volatile void) [182] */
	return arg;
}

void
init_discarding_volatile(volatile void *arg)
{
	/* expect+1: warning: incompatible pointer types (void != volatile void) [182] */
	void *array[] = { arg };
}
