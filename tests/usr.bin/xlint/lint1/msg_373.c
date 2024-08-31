/*	$NetBSD: msg_373.c,v 1.4 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_373.c"

// Test for message: bit field end %ju in '%.*s' out of range 0..64 [373]

/*
 * A bit-field may start in the middle of the value.  When its end goes beyond
 * 64, this means the uppermost bits will always be 0, and a narrower
 * bit-field would have the same effect.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(uint64_t u64)
{
	char buf[64];

	/* expect+12: warning: field width '\101' (65) in 'f\000\101all+1\0' out of range 0..64 [372] */
	/* expect+11: warning: bit field end 65 in 'f\000\101all+1\0' out of range 0..64 [373] */
	/* expect+10: warning: bit field end 65 in 'f\001\100oob64\0' out of range 0..64 [373] */
	/* expect+9: warning: 'f\001\100oob64\0' overlaps earlier 'f\000\100all\0' on bit 1 [376] */
	/* expect+8: warning: field width '\377' (255) in 'f\010\377oob64\0' out of range 0..64 [372] */
	/* expect+7: warning: bit field end 263 in 'f\010\377oob64\0' out of range 0..64 [373] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\000\100all\0"
	    "f\000\101all+1\0"
	    "f\001\100oob64\0"
	    "f\010\377oob64\0",
	    u64);

	/* expect+5: warning: bit position '\377' (255) in 'f\377\002wrap-around\0' out of range 0..63 [371] */
	/* expect+4: warning: bit field end 257 in 'f\377\002wrap-around\0' out of range 0..64 [373] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\377\002wrap-around\0",
	    u64);
}
