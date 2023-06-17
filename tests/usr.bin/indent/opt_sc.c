/* $NetBSD: opt_sc.c,v 1.8 2023/06/17 22:09:24 rillig Exp $ */

/*
 * Tests for the options '-sc' and '-nsc'.
 *
 * The option '-sc' starts continuation lines of block comments with " * ".
 *
 * The option '-nsc' does not use asterisks for aligning the continuation
 * lines of comments.
 */

//indent input
/* comment
without
asterisks
*/
//indent end

//indent run -sc
/*
 * comment without asterisks
 */
//indent end

//indent run -nsc
/*
comment without asterisks
 */
//indent end


//indent input
/*
** This comment style is used by Lua.
*/
//indent end

//indent run -sc
/*
// $ XXX: The additional '*' is debatable.
 * * This comment style is used by Lua.
 */
//indent end

//indent run -nsc
/*
// $ This comment, as rewritten by indent, is not actually used by Lua.
 * This comment style is used by Lua.
 */
//indent end


/*
 * Comments that start with '*' or '-' do not get modified at all.
 */
//indent input
/**
 * Javadoc, adopted by several other programming languages.
 */
//indent end

//indent run-equals-input -sc

//indent run-equals-input -nsc


/*
 * Ensure that blank lines in comments are preserved. Multiple adjacent blank
 * lines are preserved as well.
 */
//indent input
/*
paragraph 1


paragraph 2
*/
//indent end

//indent run -sc
/*
 * paragraph 1
 *
 *
 * paragraph 2
 */
//indent end

//indent run -nsc
/*
// $ XXX: paragraph 1 is indented, paragraph 2 isn't.
 paragraph 1


paragraph 2
 */
//indent end
