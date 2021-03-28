/*	$NetBSD: msg_176.c,v 1.3 2021/03/28 14:01:50 rillig Exp $	*/
# 3 "msg_176.c"

// Test for message: invalid initializer type %s [176]

/*
 * Before init.c 1.161 from 2021-03-28, lint wronly complained about
 * initializers with redundant braces.
 *
 * C99 allows these, both GCC and Clang warn about them since they are unusual
 * and confusing.
 */

int valid = {{{3}}};
