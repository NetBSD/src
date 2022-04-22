/* $NetBSD: opt_ei.c,v 1.6 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the options '-ei' and '-nei'.
 *
 * The option '-ei' indents the 'if' in 'else if' as part of the outer 'if'
 * statement.
 *
 * The option '-nei' treats the 'if' in 'else if' as a separate, independent
 * statement that is indented one level deeper than the outer 'if'.
 */

#indent input
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

#indent run-equals-input -ei

#indent run -nei
void
example(int n)
{
	if (n > 99) {
		print("large");
	} else
		if (n > 9) {
			print("double-digit");
		} else
			if (n > 0)
				print("positive");
			else {
				print("negative");
			}
}
#indent end

#indent run -nei -nce
void
example(int n)
{
	if (n > 99) {
		print("large");
	}
	else
		if (n > 9) {
			print("double-digit");
		}
		else
			if (n > 0)
				print("positive");
			else {
				print("negative");
			}
}
#indent end
