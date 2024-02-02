/*	$NetBSD: msg_080.c,v 1.7 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_080.c"

// Test for message: dubious escape \%o [80]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: dubious escape \11 [80] */
char char_backslash_tab = '\	';
/* expect+1: warning: dubious escape \177 [80] */
char char_backslash_delete = '\';
/* expect+1: warning: dubious escape \11 [80] */
int wide_backslash_tab = L'\	';
/* expect+1: warning: dubious escape \177 [80] */
int wide_backslash_delete = L'\';

/* expect+1: warning: dubious escape \11 [80] */
char char_string_backslash_tab[] = "\	";
/* expect+1: warning: dubious escape \177 [80] */
char char_string_backslash_delete[] = "\";
/* expect+1: warning: dubious escape \11 [80] */
int wide_string_backslash_tab[] = L"\	";
/* expect+1: warning: dubious escape \177 [80] */
int wide_string_backslash_delete[] = L"\";
