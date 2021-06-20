/*	$NetBSD: lex_char.c,v 1.2 2021/06/20 18:23:07 rillig Exp $	*/
# 3 "lex_char.c"

/*
 * Tests for lexical analysis of character constants.
 *
 * C99 6.4.4.4 "Character constants"
 */

void sink(char);

void
test(void)
{
	/* expect+1: empty character constant */
	sink('');

	sink('a');

	sink('\0');

	/* UTF-8 */
	/* expect+2: multi-character character constant */
	/* expect+1: conversion of 'int' to 'char' is out of range */
	sink('ä');

	/* GCC extension */
	/* expect+1: dubious escape \e */
	sink('\e');

	/* since C99 */
	sink('\x12');

	/* octal */
	sink('\177');

	/* newline */
	sink('\n');

	/* expect+1: empty character constant */
	sink('');
}
