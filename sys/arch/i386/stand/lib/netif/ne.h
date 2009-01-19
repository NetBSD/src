/*	$NetBSD: ne.h,v 1.2.86.1 2009/01/19 13:16:21 skrll Exp $	*/

void ne2000_readmem(int, uint8_t *, size_t);
void ne2000_writemem(uint8_t *, int, size_t);
