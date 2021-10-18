/* $NetBSD: token_period.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token '.'.
 *
 * The '.' in numbers such as 3.14159265358979 is not a token '.'.
 *
 * The token '.' is used to access a member of a struct or union.
 */


/*
 * The ellipsis for the function parameter is a sequence of three '.' tokens.
 * It would have been more intuitive to model them as a single token, but it
 * doesn't make any difference for formatting the code.
 */
#indent input
void my_printf(const char *, ...);
#indent end

#indent run-equals-input -di0


#indent input
int var = str.member;
int var = str . member;
#indent end

#indent run -di0
int var = str.member;
int var = str.member;
#indent end
