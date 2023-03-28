/*	$NetBSD: d_c99_flex_array_packed.c,v 1.3 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_flex_array_packed.c"

/* lint1-extra-flags: -X 351 */

/* Allow packed c99 flexible arrays */
struct {
	int x;
	char y[0];
} __packed foo;
