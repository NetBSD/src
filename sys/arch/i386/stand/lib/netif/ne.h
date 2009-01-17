/*	$NetBSD: ne.h,v 1.2.74.1 2009/01/17 13:28:06 mjf Exp $	*/

void ne2000_readmem(int, uint8_t *, size_t);
void ne2000_writemem(uint8_t *, int, size_t);
