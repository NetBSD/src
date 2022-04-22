/* $NetBSD: lsym_word.c,v 1.4 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the token lsym_word, which represents a constant, a string
 * literal or an identifier.
 *
 * See also:
 *	lsym_funcname.c		for an identifier followed by '('
 */

// TODO: Is '"string"(' syntactically valid in any context?
// TODO: Is '123(' syntactically valid in any context?
// TODO: Would the output of the above depend on -pcs/-npcs?

#indent input
// TODO: add input
#indent end

#indent run-equals-input


/*
 * Since 2019-04-04 and before NetBSD lexi.c 1.149 from 2021-11-20, the first
 * character after a backslash continuation was always considered part of a
 * word, no matter whether it was a word character or not.
 */
#indent input
int var\
+name = 4;
#indent end

#indent run
int		var + name = 4;
#indent end
