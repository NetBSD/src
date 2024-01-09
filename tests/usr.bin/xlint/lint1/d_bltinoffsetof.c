/*	$NetBSD: d_bltinoffsetof.c,v 1.3 2024/01/09 23:46:54 rillig Exp $	*/
# 3 "d_bltinoffsetof.c"

struct foo {
	union {
		struct {
			struct {
				int a;
				int b;
			} first;
			char *second;
		} s;
		unsigned char padding[1000];
	} u;
	union {
		int a;
		double b;
	} array[50];
};

typedef int first[-(int)__builtin_offsetof(struct foo, u.s.first)];
typedef int first_a[-(int)__builtin_offsetof(struct foo, u.s.first.a)];
/* expect+1: ... (-4) ... */
typedef int first_b[-(int)__builtin_offsetof(struct foo, u.s.first.b)];
/* expect+1: ... (-8) ... */
typedef int second[-(int)__builtin_offsetof(struct foo, u.s.second)];

/* expect+1: ... (-1000) ... */
typedef int array[-(int)__builtin_offsetof(struct foo, array)];
/* expect+1: ... (-1000) ... */
typedef int array_0_a[-(int)__builtin_offsetof(struct foo, array[0].a)];
/* expect+1: ... (-1000) ... */
typedef int array_0_b[-(int)__builtin_offsetof(struct foo, array[0].b)];
/* expect+1: ... (-1008) ... */
typedef int array_1_a[-(int)__builtin_offsetof(struct foo, array[1].a)];

// There is no element array[50], but pointing right behind the last element
// may be fine.
/* expect+1: ... (-1400) ... */
typedef int array_50_a[-(int)__builtin_offsetof(struct foo, array[50].a)];
/* expect+1: ... (-1400) ... */
typedef int sizeof_foo[-(int)sizeof(struct foo)];


// 51 is out of bounds, as it is larger than the size of the struct.
// No warning though, maybe later.
/* expect+1: ... (-1408) ... */
typedef int array_51_a[-(int)__builtin_offsetof(struct foo, array[51].a)];
