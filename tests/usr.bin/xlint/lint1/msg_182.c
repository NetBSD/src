/*	$NetBSD: msg_182.c,v 1.6 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_182.c"

// Test for message: incompatible pointer types to '%s' and '%s' [182]

/* lint1-extra-flags: -X 351 */

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
