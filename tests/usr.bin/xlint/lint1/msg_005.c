/*	$NetBSD: msg_005.c,v 1.2 2021/01/02 18:06:01 rillig Exp $	*/
# 3 "msg_005.c"

// Test for message: modifying typedef with '%s'; only qualifiers allowed [5]

typedef int number;
number long long_variable;
number const const_variable;
