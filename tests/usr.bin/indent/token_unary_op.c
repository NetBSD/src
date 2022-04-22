/* $NetBSD: token_unary_op.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for unary operators, such as '+', '-', '*', '&'.
 */

#indent input
int var=+3;
int mixed=+-+-+-+-+-+-+-+-+-+-+-+-+-3;
int count=~-~-~-~-~-~-~-~-~-~-~-~-~-3;
int same = + + + + + - - - - - 3;
#indent end

#indent run -di0
int var = +3;
int mixed = +-+-+-+-+-+-+-+-+-+-+-+-+-3;
int count = ~-~-~-~-~-~-~-~-~-~-~-~-~-3;
int same = + + + + +- - - - -3;
#indent end
