/* $NetBSD: opt_bl_br.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void
example(int n)
{
	/*
	 * XXX: The '} else' looks strange in this style since the 'else' is
	 * not at the left margin of the code.
	 */
	if (n > 99) { print("large"); }
	else if (n > 9) { print("double-digit"); }
	else if (n > 0) print("positive");
	else { print("negative"); }
}
#indent end

#indent run -bl
void
example(int n)
{
	/*
	 * XXX: The '} else' looks strange in this style since the 'else' is
	 * not at the left margin of the code.
	 */
	if (n > 99)
	{
		print("large");
	} else if (n > 9)
	{
		print("double-digit");
	} else if (n > 0)
		print("positive");
	else
	{
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

#indent run -br
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
