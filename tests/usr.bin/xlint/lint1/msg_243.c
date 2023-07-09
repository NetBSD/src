/*	$NetBSD: msg_243.c,v 1.6 2023/07/09 12:04:08 rillig Exp $	*/
# 3 "msg_243.c"

// Test for message: operator '%s' assumes that '%s' is ordered [243]

/* lint1-extra-flags: -eP -X 351 */

enum color {
	RED, GREEN, BLUE
};

void eval(_Bool);

/* TODO: There should be a way to declare an enum type as "ordered ok". */

void
example(enum color a, enum color b)
{
	/* expect+1: warning: operator '<' assumes that 'enum color' is ordered [243] */
	eval(a < b);
	/* expect+1: warning: operator '<=' assumes that 'enum color' is ordered [243] */
	eval(a <= b);
	/* expect+1: warning: operator '>' assumes that 'enum color' is ordered [243] */
	eval(a > b);
	/* expect+1: warning: operator '>=' assumes that 'enum color' is ordered [243] */
	eval(a >= b);
	eval(a == b);
	eval(a != b);
}
