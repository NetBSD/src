/*	$NetBSD: msg_135.c,v 1.4 2021/02/28 00:40:22 rillig Exp $	*/
# 3 "msg_135.c"

// Test for message: converting '%s' to '%s' may cause alignment problem [135]

/* lint1-extra-flags: -h */

unsigned
read_uint(const unsigned char **pp)
{
	unsigned val;

	val = *(const unsigned *)(*pp);
	pp += sizeof(unsigned);
	return val;
}
