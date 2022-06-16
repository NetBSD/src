/*	$NetBSD: msg_210.c,v 1.6 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_210.c"

// Test for message: enum type mismatch between '%s' and '%s' in initialization [210]

enum A {
	A1
};

enum B {
	B1
};

typedef enum {
	C1
} C;

typedef enum {
	D1
} D;

enum A a1 = A1;
/* expect+1: warning: enum type mismatch between 'enum A' and 'enum B' in initialization [210] */
enum A a2 = B1;
C c1 = C1;
/* expect+1: warning: enum type mismatch between 'enum typedef C' and 'enum typedef D' in initialization [210] */
C c2 = D1;
