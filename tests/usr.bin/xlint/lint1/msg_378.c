/*	$NetBSD: msg_378.c,v 1.3 2024/08/31 06:57:32 rillig Exp $	*/
# 3 "msg_378.c"

// Test for message: conversion '%.*s' is unreachable by input value [378]

/*
 * The typical use case of snprintb is to have a format that is specifically
 * tailored to a particular input value.  Often, a format is only used in a
 * single place.  Therefore, bits that are unreachable are redundant and may
 * hint at typos.
 */

/* lint1-extra-flags: -X 351 */

typedef typeof(sizeof(0)) size_t;
typedef unsigned long long uint64_t;

int snprintb(char *, size_t, const char *, uint64_t);

void
example(unsigned u32, uint64_t u64)
{
	char buf[64];

	/* expect+5: warning: conversion '\040bit32' is unreachable by input value [378] */
	snprintb(buf, sizeof(buf),
	    "\020"
	    "\037bit31"
	    "\040bit32",
	    u32 >> 1);

	/* expect+5: warning: conversion 'b\075bit61\0' is unreachable by input value [378] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\074bit60\0"
	    "b\075bit61\0",
	    u64 >> 3);

	/* expect+12: warning: conversion 'b\000bit0\0' is unreachable by input value [378] */
	/* expect+11: warning: conversion 'b\011bit9\0' is unreachable by input value [378] */
	/* expect+10: warning: conversion 'f\017\002bits15-16\0' is unreachable by input value [378] */
	/* expect+9: warning: conversion 'f\050\030bits40-63\0' is unreachable by input value [378] */
	snprintb(buf, sizeof(buf),
	    "\177\020"
	    "b\000bit0\0"
	    "b\010bit8\0"
	    "b\011bit9\0"
	    "f\012\002bits10-11\0"
	    "f\017\002bits15-16\0"
	    "f\050\030bits40-63\0",
	    (u32 & 0xaa55aa55) << 8);
}
