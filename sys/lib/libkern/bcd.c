/* $NetBSD: bcd.c,v 1.1.6.2 2006/04/19 04:36:01 elad Exp $ */

/*
 * Convert a single byte between (unsigned) packed bcd and binary.
 * Public domain.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0,"$NetBSD: bcd.c,v 1.1.6.2 2006/04/19 04:36:01 elad Exp $");

#include <lib/libkern/libkern.h>

unsigned int
bcdtobin(unsigned int bcd)
{

        return (((bcd >> 4) & 0x0f) * 10 + (bcd & 0x0f));
}

unsigned int
bintobcd(unsigned int bin)
{

	return ((((bin / 10) << 4) & 0xf0) | (bin % 10));
}
