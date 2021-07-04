/*	$NetBSD: msg_080.c,v 1.4 2021/07/04 13:44:43 rillig Exp $	*/
# 3 "msg_080.c"

// Test for message: dubious escape \%o [80]

/* expect+1: dubious escape \177 [80] */
char backslash_delete = '\';
