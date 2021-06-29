/*	$NetBSD: lex_char_uchar.c,v 1.2 2021/06/29 14:19:51 rillig Exp $	*/
# 3 "lex_char_uchar.c"

/*
 * Test lexical analysis of character constants on platforms where plain
 * char has the same representation as unsigned char.
 */

/* lint1-only-if uchar */

/*
 * FIXME: The warning is bogus; it must be possible to initialize a char
 *  variable with a character constant.
 * See tree.c, function convert_constant.
 */
/* expect+1: conversion of 'int' to 'char' is out of range [119] */
char ch = '\xff';
