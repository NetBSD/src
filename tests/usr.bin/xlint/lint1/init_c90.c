/*	$NetBSD: init_c90.c,v 1.3 2022/02/27 11:40:30 rillig Exp $	*/
# 3 "init_c90.c"

/*
 * Test initialization before C99.
 *
 * C90 3.5.7
 */

/* lint1-flags: -sw */

struct point {
	int x, y;
};

struct point point_c90 = { 0, 0 };
/* expect+2: warning: struct or union member name in initializer is a C99 feature [313] */
/* expect+1: warning: struct or union member name in initializer is a C99 feature [313] */
struct point point_c99 = { .x = 0, .y = 0 };

struct point points_c90[] = {{ 0, 0 }};
/* expect+1: warning: array initializer with designators is a C99 feature [321] */
struct point points_c99[] = {[3] = { 0, 0 }};


struct point
compound_literal(void) {
	/* expect+1: compound literals are a C99/GCC extension [319] */
	return (struct point){ 0, 0 };
}
