/*	$NetBSD: dkcksum.h,v 1.5.12.1 2013/06/23 06:28:50 tls Exp $	*/

uint16_t	dkcksum(const struct disklabel *);
uint16_t	dkcksum_sized(const struct disklabel *, size_t);
