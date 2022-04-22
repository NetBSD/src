/* $NetBSD: lsym_offsetof.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the token lsym_offsetof, which represents the keyword 'offsetof'
 * that starts an expression for computing the offset of a member in a struct.
 */

#indent input
size_t		offset = offsetof(struct s, member);
#indent end

#indent run-equals-input
#indent run-equals-input -bs

/*
 * The option '-pcs' forces a blank after the function name.  That option
 * applies to 'offsetof' as well, even though it is not really a function.
 */
#indent run -pcs
size_t		offset = offsetof (struct s, member);
#indent end
