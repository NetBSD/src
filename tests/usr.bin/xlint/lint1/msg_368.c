/*	$NetBSD: msg_368.c,v 1.1 2024/03/01 19:39:29 rillig Exp $	*/
# 3 "msg_368.c"

// Test for message: missing comparison value after directive '%.*s' [368]

/*
 * The directives '=' and ':' require a comparison value as their argument,
 * followed by the description and the terminating null character.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char*, size_t, const char*, uint64_t);

void
example(uint64_t val)
{
	char buf[64];

	/* expect+4: warning: missing comparison value after directive '=' [368] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "=",
	    val);

	/* expect+4: warning: missing comparison value after directive ':' [368] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    ":",
	    val);
}
