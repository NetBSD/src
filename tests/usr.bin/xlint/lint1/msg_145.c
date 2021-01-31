/*	$NetBSD: msg_145.c,v 1.3 2021/01/31 11:12:07 rillig Exp $	*/
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

	return sizeof(local_bits.one);		/* expect: 145 */
}

unsigned long
sizeof_eight(void)
{
	struct bits local_bits;

	return sizeof(local_bits.eight);	/* expect: 145 */
}
