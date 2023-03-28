/*	$NetBSD: d_c99_struct_init.c,v 1.5 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "d_c99_struct_init.c"

/* lint1-extra-flags: -X 351 */

/* C99 struct initialization */
struct {
	int i;
	char *s;
} c[] = {
	{
		.i =  2,
	},
	{
		.s =  "foo"
	},
	{
		.i =  1,
		.s = "bar"
	},
	{
		.s =  "foo",
		.i = -1
	},
};
