/*	$NetBSD: msg_079_nongcc.c,v 1.1 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_079_nongcc.c"

// Test for message: dubious escape \%c [79]

/* \e is only accepted in GCC mode. */

/* lint1-flags: -S -w -X 351 */

/* expect+1: warning: dubious escape \e [79] */
char char_e = '\e';
/* expect+1: warning: dubious escape \y [79] */
char char_y = '\y';
/* expect+1: warning: dubious escape \e [79] */
int wide_e = L'\e';
/* expect+1: warning: dubious escape \y [79] */
int wide_y = L'\y';

/* expect+1: warning: dubious escape \e [79] */
char char_string_e[] = "\e[0m";
/* expect+1: warning: dubious escape \y [79] */
char char_string_y[] = "\y[0m";
/* expect+1: warning: dubious escape \e [79] */
int wide_string_e[] = L"\e[0m";
/* expect+1: warning: dubious escape \y [79] */
int wide_string_y[] = L"\y[0m";
