#	$NetBSD: Makefile,v 1.2 2003/05/26 15:17:38 thorpej Exp $

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms
SUBDIR=  ADI_BRH_flash_0x00140000
SUBDIR+= IQ80310_flash_0x00080000
SUBDIR+= IQ80321_flash_0xf0080000
.else
# Big endian platforms
SUBDIR=  ADI_BRH_flash_0x00140000
.endif

.include <bsd.subdir.mk>
