/*	$NetBSD: msg_005.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_005.c"

// Test for message: modifying typedef with '%s'; only qualifiers allowed [5]

typedef int number;
number long long_variable;	/* expect: 5 */
number const const_variable;
