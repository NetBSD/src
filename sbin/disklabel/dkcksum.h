/*	$NetBSD: dkcksum.h,v 1.6 2013/05/03 16:05:12 matt Exp $	*/

uint16_t	dkcksum(const struct disklabel *);
uint16_t	dkcksum_sized(const struct disklabel *, size_t);
