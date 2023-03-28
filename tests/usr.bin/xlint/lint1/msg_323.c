/*	$NetBSD: msg_323.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_323.c"

// Test for message: continue in 'do ... while (0)' loop [323]

/* lint1-extra-flags: -X 351 */

void println(const char *);

/*
 * In simple cases of a do-while-0 loop, the statements 'break' and
 * 'continue' have the same effect, and 'break' is much more common.
 *
 * This is also covered by Clang-Tidy.
 */
void
simple_case(const char *p)
{
	do {
		if (p[0] == '+')
			break;
		if (p[1] == '-')
			continue;
		println("no sign");
	/* expect+1: error: continue in 'do ... while (0)' loop [323] */
	} while (0);
}

/*
 * If there is a 'switch' statement inside the do-while-0 loop, the 'break'
 * statement is tied to the 'switch' statement instead of the loop.
 */
void
nested_switch(const char *p)
{
	do {
		switch (*p) {
		case 'a':
			continue;	/* leaves the 'do while 0' */
		case 'b':
			break;		/* leaves the 'switch' */
		}
		println("b");
	/* XXX: Is that really worth an error? */
	/* expect+1: error: continue in 'do ... while (0)' loop [323] */
	} while (0);
}

/*
 * In a nested loop, the 'continue' statement is bound to the inner loop,
 * thus no warning.
 */
void
nested_for(void)
{
	do {
		for (int i = 0; i < 6; i++) {
			if (i < 3)
				continue;
		}
	} while (0);
}
