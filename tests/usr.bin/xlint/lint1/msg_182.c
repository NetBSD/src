/*	$NetBSD: msg_182.c,v 1.8 2024/11/23 00:01:48 rillig Exp $	*/
# 3 "msg_182.c"

// Test for message: '%s' discards '%s' from '%s' [182]

/* lint1-extra-flags: -X 351 */

void *
return_discarding_volatile(volatile void *arg)
{
	/* expect+1: warning: 'return' discards 'volatile' from 'pointer to volatile void' [182] */
	return arg;
}

void
init_discarding_volatile(volatile void *arg)
{
	/* expect+2: warning: 'init' discards 'volatile' from 'pointer to volatile void' [182] */
	/* expect+1: warning: 'array' set but not used in function 'init_discarding_volatile' [191] */
	void *array[] = { arg };
}
