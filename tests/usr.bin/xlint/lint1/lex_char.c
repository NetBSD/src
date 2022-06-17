/*	$NetBSD: lex_char.c,v 1.6 2022/06/17 18:54:53 rillig Exp $	*/
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
	/* expect+1: error: empty character constant [73] */
	sink('');

	sink('a');

	sink('\0');

	/* UTF-8 */
	/* expect+2: warning: multi-character character constant [294] */
	/* expect+1: warning: conversion of 'int' to 'char' is out of range, arg #1 [295] */
	sink('ä');

	/* GCC extension */
	/* expect+1: warning: dubious escape \e [79] */
	sink('\e');

	/* since C99 */
	sink('\x12');

	/* octal */
	sink('\177');

	/* expect+1: error: empty character constant [73] */
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
 * The sequence backslash-newline is handled in an early stage of
 * translation (C90 5.1.1.2 item 2, C99 5.1.1.2 item 2, C11 5.1.1.2 item 2),
 * which allows it in character literals as well.  This doesn't typically
 * occur in practice though.
 */
char ch = '\
\
\
\
\
x';
