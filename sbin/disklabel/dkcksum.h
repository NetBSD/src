/*	$NetBSD: dkcksum.h,v 1.5 2010/01/05 15:45:26 tsutsui Exp $	*/

uint16_t	dkcksum(struct disklabel *);
uint16_t	dkcksum_sized(struct disklabel *, size_t);
