/*	$NetBSD: msg_207.c,v 1.4 2024/04/19 20:59:18 rillig Exp $	*/
# 3 "msg_207.c"

// Test for message: loop not entered at top [207]

static void
/* expect+1: warning: static function 'for_loop' unused [236] */
for_loop(void)
{
	for (int i = 0; i < 10; i++)
		if (0 == 1)
			for (i = 0;
			    i < 5;
				/* expect+2: warning: loop not entered at top [207] */
				/* expect+1: warning: end-of-loop code not reached [223] */
			    i += 4)
				return;

	// XXX: Why is this different from the snippet above?
	for (int i = 0; i < 10; i++)
		if (0 == 1)
			/* expect+1: warning: statement not reached [193] */
			for (int j = 0;
			    j < 5;
			    /* expect+1: warning: end-of-loop code not reached [223] */
			    j += 4)
				return;
}

static void
/* expect+1: warning: static function 'while_loop' unused [236] */
while_loop(void)
{
	for (int i = 0; i < 10; i++)
		if (0 == 1)
			/* expect+1: warning: loop not entered at top [207] */
			while (i < 5)
				i += 4;
}

static void
/* expect+1: warning: static function 'do_loop' unused [236] */
do_loop(void)
{
	for (int i = 0; i < 10; i++)
		if (0 == 1)
			/* expect+1: warning: loop not entered at top [207] */
			do {
				i += 4;
			} while (i < 5);
}
