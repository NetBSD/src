/*	$NetBSD: msg_172.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
# 3 "msg_172.c"

// Test for message: too many struct/union initializers [172]

struct color {
	unsigned int red: 8;
	unsigned int green: 8;
	unsigned int blue: 8;
};

struct color white = { 255, 255, 255, 255 };
