/*	$NetBSD: msg_210.c,v 1.5 2021/02/28 12:40:00 rillig Exp $	*/
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
enum A a2 = B1;			/* expect: 210 */
C c1 = C1;
C c2 = D1;			/* expect: 210 */
