/* $NetBSD: lsym_postfix_op.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the token lsym_postfix_op, which represents the operators '++'
 * and '--' for incrementing and decrementing a variable.
 *
 * See also:
 *	lsym_unary_op.c		for the corresponding prefix operators
 */

#indent input
int decl = lvalue ++;
int decl = lvalue --;
#indent end

#indent run -di0
int decl = lvalue++;
int decl = lvalue--;
#indent end


/*
 * There is no operator '**', so try that just for fun.
 */
#indent input
int decl = lvalue **;
#indent end

#indent run -di0
int decl = lvalue * *;
#indent end
