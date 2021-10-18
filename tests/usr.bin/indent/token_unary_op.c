/* $NetBSD: token_unary_op.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

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
/* $ FIXME: There must be spaces between adjacent '+'. */
/* $ FIXME: There must be spaces between adjacent '-'. */
int same = +++++-----3;
#indent end
