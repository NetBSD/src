/*	$NetBSD: msg_377.c,v 1.1 2024/03/03 00:50:41 rillig Exp $	*/
# 3 "msg_377.c"

// Test for message: redundant '\0' at the end of new-style format [377]

/*
 * Each directive in the new-style format ends with a '\0'. The final '\0'
 * that ends the whole format is provided implicitly by the compiler as part
 * of the string literal.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char*, size_t, const char*, uint64_t);

void
example(unsigned u32, uint64_t u64)
{
	char buf[64];

	/* expect+6: warning: old-style format contains '\0' [362] */
	/* expect+5: warning: empty description in '\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\020"
	    "\005bit"
	    "\0",
	    u32);

	/* expect+5: warning: redundant '\0' at the end of new-style format [377] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\005bit\0"
	    "\0",
	    u64);
}
