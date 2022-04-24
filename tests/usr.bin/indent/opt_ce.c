/* $NetBSD: opt_ce.c,v 1.6 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the options '-ce' and '-nce'.
 *
 * The option '-ce' places the 'else' on the same line as the preceding '}'.
 *
 * The option '-nce' places the 'else' on the next line.
 *
 * See also:
 *	opt_ei.c
 */

//indent input
void
example(int n)
{
	if (n > 99) { print("large"); }
	else if (n > 9) { print("double-digit"); }
	else if (n > 0) print("positive");
	else { print("negative"); }
}
//indent end

//indent run -ce
void
example(int n)
{
	if (n > 99) {
		print("large");
	} else if (n > 9) {
		print("double-digit");
	} else if (n > 0)
		print("positive");
	else {
		print("negative");
	}
}
//indent end

//indent run -nce
void
example(int n)
{
	if (n > 99) {
		print("large");
	}
	else if (n > 9) {
		print("double-digit");
	}
	else if (n > 0)
		print("positive");
	else {
		print("negative");
	}
}
//indent end
