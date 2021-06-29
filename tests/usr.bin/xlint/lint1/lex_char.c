/*	$NetBSD: lex_char.c,v 1.4 2021/06/29 07:28:01 rillig Exp $	*/
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

	/* expect+1: empty character constant */
	sink('');

	/* U+0007 alarm/bell */
	sink('\a');

	/* U+0008 backspace */
	sink('\b');

	/* U+0009 horizontal tabulation */
	sink('\t');

	/* U+000A line feed */
	sink('\n');

	/* U+000B vertical tabulation */
	sink('\v');

	/* U+000C form feed */
	sink('\f');

	/* U+000D carriage return */
	sink('\r');
}

/*
 * Even though backslash-newline is not supported by C99, lint accepts it
 * in any mode, even for traditional C.
 */
char ch = '\
\
\
\
\
x';
