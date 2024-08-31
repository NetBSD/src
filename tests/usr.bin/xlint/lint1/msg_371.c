/*	$NetBSD: msg_371.c,v 1.3 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_371.c"

// Test for message: bit position '%.*s' (%ju) in '%.*s' out of range %u..%u [371]

/*
 * In old-style formats, bit positions are 1-based and must be in the range
 * from 1 to 32.  In new-style formats, bit positions are 0-based and must be
 * in the range from 0 to 63.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(unsigned u32, uint64_t u64)
{
	char buf[64];

	/* expect+12: warning: bit position '\000' (0) in '\000zero' out of range 1..32 [371] */
	/* expect+11: warning: escaped character '\041' in description of conversion '\040bit32""\041' [363] */
	/* expect+10: warning: escaped character '\177' in description of conversion '\040bit32""\041bit33""\177' [363] */
	/* expect+9: warning: escaped character '\377' in description of conversion '\040bit32""\041bit33""\177bit127""\377' [363] */
	snprintb(buf, sizeof(buf),
	    "\020"
	    "\000zero"
	    "\001bit1"
	    "\040bit32"
	    "\041bit33"
	    "\177bit127"
	    "\377bit255",
	    u32);

	/* expect+10: warning: bit position '\100' (64) in 'b\100bit64\0' out of range 0..63 [371] */
	/* expect+9: warning: bit position '\177' (127) in 'b\177bit127\0' out of range 0..63 [371] */
	/* expect+8: warning: bit position '\377' (255) in 'b\377bit255\0' out of range 0..63 [371] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\000bit0\0"
	    "b\077bit63\0"
	    "b\100bit64\0"
	    "b\177bit127\0"
	    "b\377bit255\0",
	    u64);

	/* expect+11: warning: bit position '\100' (64) in 'F\100\000none\0' out of range 0..63 [371] */
	/* expect+10: warning: bit position '\100' (64) in 'f\100\001oob\0' out of range 0..63 [371] */
	/* expect+9: warning: bit field end 65 in 'f\100\001oob\0' out of range 0..64 [373] */
	/* expect+8: warning: bit position '\101' (65) in 'F\101\001oob\0' out of range 0..63 [371] */
	/* expect+7: warning: bit field end 66 in 'F\101\001oob\0' out of range 0..64 [373] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\077\001msb\0"
	    "F\100\000none\0"
	    "f\100\001oob\0"
	    "F\101\001oob\0",
	    u64);
}
