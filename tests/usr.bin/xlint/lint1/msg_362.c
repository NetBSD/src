/*	$NetBSD: msg_362.c,v 1.1 2024/03/01 19:39:28 rillig Exp $	*/
# 3 "msg_362.c"

// Test for message: old-style format contains '\0' [362]

/*
 * The old-style format uses 1-based bit positions, from 1 up to 32.
 * A null character would prematurely end the format argument.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char*, size_t, const char*, uint64_t);

void
example(unsigned u32)
{
	char buf[64];

	/* expect+1: warning: bit position '\000' (0) in '\000lsb' out of range 1..32 [371] */
	snprintb(buf, sizeof(buf), "\020\000lsb\037msb", u32);
	/* expect+2: warning: old-style format contains '\0' [362] */
	/* expect+1: warning: bit position '\000' (0) in '\000lsb' out of range 1..32 [371] */
	snprintb(buf, sizeof(buf), "\020\037msb\000lsb", u32);
}
