/*	$NetBSD: msg_172.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_172.c"

// Test for message: too many struct/union initializers [172]

struct color {
	unsigned int red: 8;
	unsigned int green: 8;
	unsigned int blue: 8;
};

struct color white = { 255, 255, 255, 255 };	/* expect: 172 */
