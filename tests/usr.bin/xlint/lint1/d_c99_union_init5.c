/* $NetBSD: d_c99_union_init5.c,v 1.2 2023/03/28 14:44:34 rillig Exp $ */
# 3 "d_c99_union_init5.c"

/*
 * PR bin/20264: lint(1) has problems with named member initialization
 *
 * Has been fixed somewhere between 2005.12.24.20.47.56 and
 * 2006.12.19.19.06.44.
*/

/* lint1-extra-flags: -X 351 */

union mist {
	char *p;
	int a[1];
};

union mist xx = {
    .a = { 7 }
};
