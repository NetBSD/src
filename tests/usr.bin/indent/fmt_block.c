/* $NetBSD: fmt_block.c,v 1.1 2021/10/22 19:27:53 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void
function(void)
{
	if (true) {

	}

	{
		print("block");
	}
}
#indent end

#indent run
void
function(void)
{
	if (true) {

/* $ FIXME: indent must not merge these braces. */
	} {
/* $ FIXME: the following empty line was not in the input. */

		print("block");
	}
}
#indent end
