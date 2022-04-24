/* $NetBSD: lsym_rparen_or_rbracket.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token lsym_rparen_or_lbracket, which represents ')' or ']',
 * the counterparts for '(' and '['.
 *
 * See also:
 *	lsym_lparen_or_lbracket.c
 */

//indent input
int var = (3);
int cast = (int)3;
int cast = (int)(3);
int call = function(3);
int array[3] = {1, 2, 3};
int array[3] = {[2] = 3};
//indent end

//indent run-equals-input -di0
