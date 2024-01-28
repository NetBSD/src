/*	$NetBSD: msg_182.c,v 1.7 2024/01/28 08:17:27 rillig Exp $	*/
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
	/* expect+2: warning: incompatible pointer types to 'void' and 'volatile void' [182] */
	/* expect+1: warning: 'array' set but not used in function 'init_discarding_volatile' [191] */
	void *array[] = { arg };
}
