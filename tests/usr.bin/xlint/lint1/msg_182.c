/*	$NetBSD: msg_182.c,v 1.5 2023/07/07 06:03:31 rillig Exp $	*/
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
	/* expect+2: warning: 'array' set but not used in function 'init_discarding_volatile' [191] */
	/* expect+1: warning: incompatible pointer types to 'void' and 'volatile void' [182] */
	void *array[] = { arg };
}
