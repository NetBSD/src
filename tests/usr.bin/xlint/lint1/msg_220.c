/*	$NetBSD: msg_220.c,v 1.4 2021/08/29 08:57:50 rillig Exp $	*/
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

/* https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wimplicit-fallthrough */
void
annotation_comment_variations(int n)
{
	switch (n) {
	case 0:
		println("0");
		/* FALLTHROUGH */
	case 1:
		println("1");
		/* FALL THROUGH */
		/* expect+1: warning: fallthrough on case statement [220] */
	case 2:
		println("2");
		/* FALLS THROUGH */
		/* expect+1: warning: fallthrough on case statement [220] */
	case 3:
		println("3");
		/* intentionally falls through */
		/* expect+1: warning: fallthrough on case statement [220] */
	case 4:
		println("4");
		/* @fallthrough@ */
		/* expect+1: warning: fallthrough on case statement [220] */
	case 5:
		println("5");
	}
}
