/*	$NetBSD: msg_172.c,v 1.5 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_172.c"

// Test for message: too many struct/union initializers [172]

/* lint1-extra-flags: -X 351 */

struct color {
	unsigned int red: 8;
	unsigned int green: 8;
	unsigned int blue: 8;
};

/* expect+1: error: too many struct/union initializers [172] */
struct color white = { 255, 255, 255, 255 };
