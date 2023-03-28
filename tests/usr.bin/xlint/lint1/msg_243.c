/*	$NetBSD: msg_243.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_243.c"

// Test for message: dubious comparison of enums, op '%s' [243]

/* lint1-extra-flags: -eP -X 351 */

enum color {
	RED, GREEN, BLUE
};

void eval(_Bool);

/* TODO: There should be a way to declare an enum type as "ordered ok". */

void
example(enum color a, enum color b)
{
	/* expect+1: warning: dubious comparison of enums, op '<' [243] */
	eval(a < b);
	/* expect+1: warning: dubious comparison of enums, op '<=' [243] */
	eval(a <= b);
	/* expect+1: warning: dubious comparison of enums, op '>' [243] */
	eval(a > b);
	/* expect+1: warning: dubious comparison of enums, op '>=' [243] */
	eval(a >= b);
	eval(a == b);
	eval(a != b);
}
