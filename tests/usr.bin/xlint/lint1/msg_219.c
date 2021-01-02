/*	$NetBSD: msg_219.c,v 1.2 2021/01/02 11:12:34 rillig Exp $	*/
# 3 "msg_219.c"


/* Test for message: concatenated strings are illegal in traditional C [219] */

/* lint1-flags: -t -w */

char concat1[] = "one";
char concat2[] = "one" "two";
char concat3[] = "one" "two" "three";
char concat4[] = "one" "two" "three" "four";
