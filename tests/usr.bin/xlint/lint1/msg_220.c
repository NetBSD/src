/*	$NetBSD: msg_220.c,v 1.2 2021/01/30 17:02:58 rillig Exp $	*/
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
