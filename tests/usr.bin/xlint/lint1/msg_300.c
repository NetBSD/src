/*	$NetBSD: msg_300.c,v 1.5 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_300.c"

/* Test for message: old-style definition [300] */

/* lint1-flags: -wr -X 351 */

/* expect+1: old-style definition [300] */
old_style(x)
	double x;
{
	return x > 0.0;
}

/* expect+1: error: prototype does not match old-style definition, arg #1 [299] */
void old_style(char ch);
