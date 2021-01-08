/*	$NetBSD: msg_145.c,v 1.2 2021/01/08 21:25:03 rillig Exp $	*/
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

	return sizeof(local_bits.one);
}

unsigned long
sizeof_eight(void)
{
	struct bits local_bits;

	return sizeof(local_bits.eight);
}
