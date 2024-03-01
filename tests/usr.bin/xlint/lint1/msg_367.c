/*	$NetBSD: msg_367.c,v 1.1 2024/03/01 19:39:28 rillig Exp $	*/
# 3 "msg_367.c"

// Test for message: empty description in '%.*s' [367]

/*
 * Each bit or field or comparison value gets a description.  If such a
 * description is empty, the generated output will contain empty angle
 * brackets or multiple adjacent commas or commas adjacent to an angle
 * bracket, such as '<,,,,>'.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char*, size_t, const char*, uint64_t);

void
example(unsigned u32)
{
	char buf[64];

	/* expect+4: warning: empty description in 'b\000\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\000\0",
	    u32);

	/* expect+4: warning: empty description in 'f\000\010\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\000\010\0",
	    u32);

	// No warning, as 'F' does not take a description.
	// If there were a description, it would simply be skipped.
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "F\000\010\0",
	    u32);

	/* expect+4: warning: empty description in '=\000\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "=\000\0",
	    u32);

	/* expect+4: warning: empty description in ':\000\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    ":\000\0",
	    u32);

	/* expect+4: warning: empty description in '*\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "*\0",
	    u32);

}
