/*	$NetBSD: msg_220.c,v 1.3 2021/04/12 15:54:55 christos Exp $	*/
# 3 "msg_220.c"

// Test for message: fallthrough on case statement [220]

/* lint1-extra-flags: -h */

extern void
println(const char *);

void
example(int n)
{
	switch (n) {
	case 1:
	case 3:
	case 5:
		println("odd");
	case 2:			/* expect: 220 */
	case 7:
		println("prime");
	default:		/* expect: 284 */
		println("number");
	}
}

void
example1(int n)
{
	switch (n) {
	case 1:
	case 3:
	case 5:
		println("odd");
		__attribute__((__fallthrough__));
	case 2:
	case 7:
		println("prime");
		__attribute__((__fallthrough__));
	default:
		println("number");
	}
}
