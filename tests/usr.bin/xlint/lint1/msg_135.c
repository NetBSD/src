/*	$NetBSD: msg_135.c,v 1.5 2021/02/28 01:20:54 rillig Exp $	*/
# 3 "msg_135.c"

// Test for message: converting '%s' to '%s' may cause alignment problem [135]

/* lint1-extra-flags: -h */

unsigned
read_uint(const unsigned char **pp)
{
	unsigned val;

	val = *(const unsigned *)(*pp);	/* expect: 135 */
	pp += sizeof(unsigned);
	return val;
}
