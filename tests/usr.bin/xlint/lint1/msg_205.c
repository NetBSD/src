/*	$NetBSD: msg_205.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_205.c"

// Test for message: switch expression must have integral type [205]

/* ARGSUSED */
void
example(double x)
{
	/* expect+1: error: switch expression must have integral type [205] */
	switch (x) {
	}
}
