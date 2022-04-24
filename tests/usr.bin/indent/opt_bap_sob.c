/* $NetBSD: opt_bap_sob.c,v 1.4 2022/04/24 09:04:12 rillig Exp $ */

/*
 * As of 2021-03-08, the combination of -bap and -sob, which occurs in the
 * example indent.pro from NetBSD, removes the empty line above the
 * separator.  Seen in games/cgram/cgram.c.
 */

//indent input
void
function1(void)
{
}

///// C99 separator /////

void
function2(void)
{
}

/* C block separator */

void
function3(void)
{
}
//indent end

//indent run -bap -sob
void
function1(void)
{
}
/* $ FIXME: Keep the empty line between the '}' and the '//'. */
///// C99 separator /////

void
function2(void)
{
}
/* $ FIXME: Keep the empty line. */
/* C block separator */

void
function3(void)
{
}
//indent end

/*
 * XXX: Strangely, the option '-nbap' keeps the empty lines after the
 * function bodies.  That's exactly the opposite of the behavior that's
 * described in the manual.
 */
//indent run-equals-input -nbap -sob

/*
 * Without '-sob', the option '-bap' works as intended.
 */
//indent run-equals-input -bap
