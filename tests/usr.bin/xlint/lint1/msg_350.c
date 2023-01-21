/*	$NetBSD: msg_350.c,v 1.1 2023/01/21 13:07:22 rillig Exp $	*/
# 3 "msg_350.c"

// Test for message 350: '_Atomic' requires C11 or later [350]

/* expect+1: error: '_Atomic' requires C11 or later [350] */
typedef _Atomic int atomic_int;

/* expect+1: error: '_Atomic' requires C11 or later [350] */
typedef _Atomic struct {
	int field;
} atomic_struct;
