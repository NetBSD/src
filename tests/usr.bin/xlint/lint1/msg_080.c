/*	$NetBSD: msg_080.c,v 1.5 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_080.c"

// Test for message: dubious escape \%o [80]

/* expect+1: warning: dubious escape \177 [80] */
char backslash_delete = '\';
