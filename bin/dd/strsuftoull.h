/*	$NetBSD: strsuftoull.h,v 1.2 2001/11/26 00:13:24 lukem Exp $	*/

uint64_t strsuftoull(const char *, const char *, uint64_t, uint64_t);

uint64_t strsuftoullx(const char *, const char *, uint64_t, uint64_t,
	    char *, size_t);
