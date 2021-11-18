/* $NetBSD: lsym_word.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

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
