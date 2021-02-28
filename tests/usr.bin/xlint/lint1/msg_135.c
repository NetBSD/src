/*	$NetBSD: msg_135.c,v 1.3 2021/02/28 00:20:19 rillig Exp $	*/
# 3 "msg_135.c"

// Test for message: possible pointer alignment problem [135]

/* lint1-extra-flags: -h */

unsigned
read_uint(const unsigned char **pp)
{
	unsigned val;

	val = *(const unsigned *)(*pp);
	pp += sizeof(unsigned);
	return val;
}
