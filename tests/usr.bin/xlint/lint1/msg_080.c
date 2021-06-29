/*	$NetBSD: msg_080.c,v 1.3 2021/06/29 07:17:43 rillig Exp $	*/
# 3 "msg_080.c"

// Test for message: dubious escape \%o [80]

/* expect+3: dubious escape \342 [80] */ /* FIXME: Why 342? */
/* expect+2: multi-character character constant [294] */
/* expect+1: initializer does not fit */
char backslash_delete = '\⌂';
