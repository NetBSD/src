/*	$NetBSD: msg_323.c,v 1.3 2021/10/09 21:25:39 rillig Exp $	*/
# 3 "msg_323.c"

// Test for message: continue in 'do ... while (0)' loop [323]
void println(const char *);

void
example(const char *p)
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
