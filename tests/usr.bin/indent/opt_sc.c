/* $NetBSD: opt_sc.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
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
