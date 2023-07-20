/*	$NetBSD: msg_205.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_205.c"

// Test for message: switch expression must have integral type [205]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
void
example(double x)
{
	/* expect+1: error: switch expression must have integral type [205] */
	switch (x) {
	}
}
