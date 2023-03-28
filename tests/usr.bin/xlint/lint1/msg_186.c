/*	$NetBSD: msg_186.c,v 1.6 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_186.c"

/* Test for message: bit-field initialization is illegal in traditional C [186] */

/* lint1-flags: -tw -X 351 */

struct bit_field {
	unsigned one: 1;
	unsigned three: 3;
	unsigned two: 2;
};

struct bit_field bit_field = {
	1,
	/* expect+1: warning: bit-field initialization is illegal in traditional C [186] */
	3.0,
	2
};
/* XXX: The message is misleading.  Initialization using integers is ok. */
