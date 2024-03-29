/*	$NetBSD: msg_186.c,v 1.7 2024/03/29 07:35:45 rillig Exp $	*/
# 3 "msg_186.c"

/* Test for message: bit-field initializer must be an integer in traditional C [186] */

/* lint1-flags: -tw -X 351 */

struct bit_field {
	unsigned one: 1;
	unsigned three: 3;
	unsigned two: 2;
};

struct bit_field bit_field = {
	1,
	/* expect+1: warning: bit-field initializer must be an integer in traditional C [186] */
	3.0,
	2
};
