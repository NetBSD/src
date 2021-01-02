/*	$NetBSD: msg_085.c,v 1.2 2021/01/02 18:06:01 rillig Exp $	*/
# 3 "msg_085.c"

// Test for message: dubious tag declaration: %s %s [85]

extern int stat(struct stat *);

struct ok;
extern int ok(struct ok *);
