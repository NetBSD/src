/* $NetBSD: opt_sc.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

#indent input
/* comment
without
asterisks
*/

/*
** This comment style is used by Lua.
*/

/**
 * Javadoc, adopted by several other programming languages.
 */
#indent end

#indent run -sc
/*
 * comment without asterisks
 */

/* $ XXX: The additional '*' is debatable. */
/*
 * * This comment style is used by Lua.
 */

/**
 * Javadoc, adopted by several other programming languages.
 */
#indent end

#indent input
/* comment
without
asterisks
*/

/*
** This comment style is used by Lua.
*/

/**
 * Javadoc, adopted by several other programming languages.
 */
#indent end

#indent run -nsc
/*
comment without asterisks
 */

/* $ This comment, as rewritten by indent, is not actually used by Lua. */
/*
 * This comment style is used by Lua.
 */

/**
 * Javadoc, adopted by several other programming languages.
 */
#indent end
