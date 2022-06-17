/*	$NetBSD: msg_311.c,v 1.3 2022/06/17 06:59:16 rillig Exp $	*/
# 3 "msg_311.c"

// Test for message: symbol renaming can't be used on automatic variables [311]

typedef int (*callback)(void);

callback
example(void)
{
	int (*func)(void) __symbolrename(function);

	func = (void *)0;
	return func;
}

/* expect+1: error: syntax error ':' [249] */
TODO: "Add example code that triggers the above message."
TODO: "Add example code that almost triggers the above message."
