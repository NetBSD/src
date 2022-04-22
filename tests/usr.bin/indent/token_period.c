/* $NetBSD: token_period.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

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
