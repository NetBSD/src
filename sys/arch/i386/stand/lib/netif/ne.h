/*	$NetBSD: ne.h,v 1.2.78.1 2009/05/04 08:11:19 yamt Exp $	*/

void ne2000_readmem(int, uint8_t *, size_t);
void ne2000_writemem(uint8_t *, int, size_t);
