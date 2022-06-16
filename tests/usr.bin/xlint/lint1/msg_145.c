/*	$NetBSD: msg_145.c,v 1.4 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_145.c"

// Test for message: cannot take size/alignment of bit-field [145]

struct bits {
	unsigned one: 1;
	unsigned eight: 8;
};

unsigned long
sizeof_one(void)
{
	struct bits local_bits;

	/* expect+1: error: cannot take size/alignment of bit-field [145] */
	return sizeof(local_bits.one);
}

unsigned long
sizeof_eight(void)
{
	struct bits local_bits;

	/* expect+1: error: cannot take size/alignment of bit-field [145] */
	return sizeof(local_bits.eight);
}
