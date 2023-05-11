/* $NetBSD: opt_bap_sob.c,v 1.5 2023/05/11 18:13:55 rillig Exp $ */

/*
 * Before 2023-05-11, the combination of -bap and -sob, which occurs in the
 * example indent.pro from NetBSD, removed the empty line above the
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

//indent run-equals-input -bap -sob

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
