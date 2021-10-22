/* $NetBSD: lex_char.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Test lexing of character constants.
 */

#indent input
int simple = 'x';
int multi = 'xy';
int empty = '';
int null = '\0';
int escape_hex = '\x3f';
int escape_octal = '\040';
int escape_a = '\a';
int escape_b = '\b';
int escape_f = '\f';
int escape_n = '\n';
int escape_t = '\t';
int escape_v = '\v';
int escape_single_quote = '\'';
int escape_double_quote = '\"';
int escape_backslash = '\\';
#indent end

#indent run-equals-input -di0
