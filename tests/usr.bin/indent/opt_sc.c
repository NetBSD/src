/* $NetBSD: opt_sc.c,v 1.6 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the options '-sc' and '-nsc'.
 *
 * The option '-sc' starts continuation lines of block comments with " * ".
 *
 * The option '-nsc' does not use asterisks for aligning the continuation
 * lines of comments.
 */

#indent input
/* comment
without
asterisks
*/
#indent end

#indent run -sc
/*
 * comment without asterisks
 */
#indent end

#indent run -nsc
/*
comment without asterisks
 */
#indent end


#indent input
/*
** This comment style is used by Lua.
*/
#indent end

/* XXX: The additional '*' is debatable. */
#indent run -sc
/*
 * * This comment style is used by Lua.
 */
#indent end

/* This comment, as rewritten by indent, is not actually used by Lua. */
#indent run -nsc
/*
 * This comment style is used by Lua.
 */
#indent end


/*
 * Comments that start with '*' or '-' do not get modified at all.
 */
#indent input
/**
 * Javadoc, adopted by several other programming languages.
 */
#indent end

#indent run-equals-input -sc

#indent run-equals-input -nsc
