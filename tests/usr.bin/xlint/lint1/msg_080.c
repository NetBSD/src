/*	$NetBSD: msg_080.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_080.c"

// Test for message: dubious escape \%o [80]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: dubious escape \177 [80] */
char backslash_delete = '\';
