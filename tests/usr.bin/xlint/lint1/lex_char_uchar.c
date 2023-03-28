/*	$NetBSD: lex_char_uchar.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "lex_char_uchar.c"

/*
 * Test lexical analysis of character constants on platforms where plain
 * char has the same representation as unsigned char.
 */

/* lint1-only-if: uchar */
/* lint1-extra-flags: -X 351 */

/*
 * Before inittyp.c 1.23 from 2021-06-29, the following initialization
 * triggered a wrong warning "conversion of 'int' to 'char' is out of range",
 * but only on platforms where char has the same representation as unsigned
 * char.  There are only few of these platforms, which allowed this bug to
 * survive for almost 26 years, since the initial commit of lint on
 * 1995-07-03.
 */
char ch = '\xff';
