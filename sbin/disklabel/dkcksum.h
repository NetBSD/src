/*	$NetBSD: dkcksum.h,v 1.5.6.1 2014/05/22 11:37:27 yamt Exp $	*/

uint16_t	dkcksum(const struct disklabel *);
uint16_t	dkcksum_sized(const struct disklabel *, size_t);
