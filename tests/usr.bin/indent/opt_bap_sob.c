/* $NetBSD: opt_bap_sob.c,v 1.1 2021/10/23 20:30:23 rillig Exp $ */
/* $FreeBSD$ */

/*
 * As of 2021-03-08, the combination of -bap and -sob, which occurs in the
 * example indent.pro from NetBSD, removes the empty line above the
 * separator.  Seen in games/cgram/cgram.c.
 */

#indent input
void
function1(void)
{
}

///// separator /////

void
function2(void)
{
}
#indent end

#indent run -bap -sob
void
function1(void)
{
}
/* $ FIXME: Keep the empty line between the '}' and the '//'. */
///// separator /////

void
function2(void)
{
}
#indent end
