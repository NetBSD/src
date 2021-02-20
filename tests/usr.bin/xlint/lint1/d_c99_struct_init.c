/*	$NetBSD: d_c99_struct_init.c,v 1.4 2021/02/20 22:31:20 rillig Exp $	*/
# 3 "d_c99_struct_init.c"

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
