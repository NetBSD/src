/*	$NetBSD: msg_176.c,v 1.5 2021/08/26 19:23:25 rillig Exp $	*/
# 3 "msg_176.c"

// Test for message: invalid initializer type %s [176]
/* This message is not used. */

/*
 * Before init.c 1.161 from 2021-03-28, lint wrongly complained about
 * initializers with redundant braces.
 *
 * C99 allows these, both GCC and Clang warn about them since they are unusual
 * and confusing.
 */

int valid = {{{3}}};
