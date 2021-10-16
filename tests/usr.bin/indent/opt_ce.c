/* $NetBSD: opt_ce.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-ce' and '-nce'.
 *
 * The option '-ce' places the 'else if' in a single line, as if it were only
 * a single keyword.
 *
 * The option '-nce' places the 'else' on the next line.
 */

#indent input
void
example(int n)
{
	if (n > 99) { print("large"); }
	else if (n > 9) { print("double-digit"); }
	else if (n > 0) print("positive");
	else { print("negative"); }
}
#indent end

#indent run -ce
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
#indent end

#indent run -nce
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
#indent end
