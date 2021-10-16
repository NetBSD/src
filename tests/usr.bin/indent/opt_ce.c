/* $NetBSD: opt_ce.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

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
