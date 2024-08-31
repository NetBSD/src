/*	$NetBSD: msg_375.c,v 1.4 2024/08/31 06:57:31 rillig Exp $	*/
# 3 "msg_375.c"

// Test for message: comparison value '%.*s' (%ju) exceeds maximum field value %ju [375]

/*
 * When a bit field can take the values 0 to 15, there is no point comparing
 * it to 16.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(uint64_t u64)
{
	char buf[64];

	/* expect+14: warning: comparison value '\020' (16) exceeds maximum field value 15 [375] */
	/* expect+13: warning: comparison value '\377' (255) exceeds maximum field value 15 [375] */
	/* expect+12: warning: comparison value '\020' (16) exceeds maximum field value 15 [375] */
	/* expect+11: warning: comparison value '\377' (255) exceeds maximum field value 15 [375] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "f\000\004low\0"
		"=\01715\0"
		"=\02016\0"
		"=\37716\0"
	    "F\004\004low\0"
		":\01715\0"
		":\02016\0"
		":\37716\0",
	    u64);
}
