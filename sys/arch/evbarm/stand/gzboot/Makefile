#	$NetBSD: Makefile,v 1.3 2003/07/30 18:54:21 bsh Exp $

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms
SUBDIR=  ADI_BRH_flash_0x00140000
SUBDIR+= IQ80310_flash_0x00080000
SUBDIR+= IQ80321_flash_0xf0080000
SUBDIR+= SMDK2800_flash_0x00000000
SUBDIR+= SMDK2800_flash_0x00100000
.else
# Big endian platforms
SUBDIR=  ADI_BRH_flash_0x00140000
.endif

.include <bsd.subdir.mk>
