/*	$NetBSD: msg_367.c,v 1.3 2024/08/31 06:57:31 rillig Exp $	*/
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

int snprintb(char *, size_t, const char *, uint64_t);

void
old_style(unsigned u32)
{
	char buf[64];

	/* expect+10: warning: empty description in '\001' [367] */
	/* expect+9: warning: empty description in '\002' [367] */
	/* expect+8: warning: empty description in '\003' [367] */
	/* expect+7: warning: empty description in '\004' [367] */
	snprintb(buf, sizeof(buf),
	    "\020"
	    "\001"
	    "\002"
	    "\003"
	    "\004",
	    u32);

	/* expect+10: warning: empty description in '\001' [367] */
	/* expect+9: warning: empty description in '\002' [367] */
	/* expect+8: warning: empty description in '\003' [367] */
	/* expect+7: warning: empty description in '\004' [367] */
	snprintb(buf, sizeof(buf),
	    "\020"
	    "\001" "" ""
	    "\002" "" ""
	    "\003" "" ""
	    "\004" "" "",
	    u32);

	// Single-letter descriptions are not empty.
	snprintb(buf, sizeof(buf),
	    "\020"
	    "\001a"
	    "\002b"
	    "\003c"
	    "\004d",
	    u32);
}

void
new_style(uint64_t u64)
{
	char buf[64];

	/* expect+4: warning: empty description in 'b\000\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\000\0",
	    u64);

	/* expect+4: warning: empty description in 'f\000\010\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\000\010\0",
	    u64);

	// No warning, as 'F' does not take a description.
	// If there were a description, it would simply be skipped.
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "F\000\010\0",
	    u64);

	/* expect+4: warning: empty description in '=\000\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "=\000\0",
	    u64);

	/* expect+4: warning: empty description in ':\000\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    ":\000\0",
	    u64);

	/* expect+4: warning: empty description in '*\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "*\0",
	    u64);

	// Single-letter descriptions are not empty.
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\000b\0"
	    "f\001\001f\0"
	    "F\002\002F\0"
		"=\000z\0"
		":\001o\0"
		"*d\0",
	    u64 >> 1);

	/* expect+6: warning: empty description in 'b\001""""""\0' [367] */
	/* expect+5: warning: empty description in 'b\003""""""\0' [367] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\001" "" "" "\0"
	    "b\003" "" "" "\0",
	    u64);
}
