/*	$NetBSD: msg_376.c,v 1.4 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_376.c"

// Test for message: '%.*s' overlaps earlier '%.*s' on bit %u [376]

/*
 * When bits and fields overlap, it's often due to typos or off-by-one errors.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(unsigned u32, uint64_t u64)
{
	char buf[64];

	// In the old-style format, bit positions are 1-based.
	snprintb(buf, sizeof(buf),
	    "\020"
	    "\001lsb"
	    "\x01lsb"
	    "\040msb"
	    "\x20msb"
	    "\041oob"
	    "\x21oob",
	    /* expect+4: warning: '\x01lsb' overlaps earlier '\001lsb' on bit 1 [376] */
	    /* expect+3: warning: escaped character '\041' in description of conversion '\x20msb""\041' [363] */
	    /* expect+2: warning: escaped character '\x21' in description of conversion '\x20msb""\041oob""\x21' [363] */
	    /* expect+1: warning: '\x20msb""\041oob""\x21oob' overlaps earlier '\040msb' on bit 32 [376] */
	    u32);

	// In the new-style format, bit positions are 0-based.
	/* expect+10: warning: 'b\x00lsb\0' overlaps earlier 'b\000lsb\0' on bit 0 [376] */
	/* expect+9: warning: 'b\x3fmsb\0' overlaps earlier 'b\077msb\0' on bit 63 [376] */
	/* expect+8: warning: bit position '\x40' (64) in 'b\x40oob\0' out of range 0..63 [371] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\000lsb\0"
	    "b\x00lsb\0"
	    "b\077msb\0"
	    "b\x3fmsb\0"
	    "b\x40oob\0",
	    u64);

	/* expect+7: warning: 'F\014\010f2\0' overlaps earlier 'f\010\010f1\0' on bit 12 [376] */
	/* expect+6: warning: 'f\020\010f3\0' overlaps earlier 'F\014\010f2\0' on bit 16 [376] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\010\010f1\0"
	    "F\014\010f2\0"
	    "f\020\010f3\0",
	    u64);
}
