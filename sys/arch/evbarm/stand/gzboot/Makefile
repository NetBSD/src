#	$NetBSD: Makefile,v 1.10 2012/06/02 20:50:41 skrll Exp $

.if ${MACHINE_ARCH} == "arm"
# Little endian platforms
SUBDIR=  ADI_BRH_flash_0x00140000
SUBDIR+= GEMINI_dram_0x01600000
SUBDIR+= IQ80310_flash_0x00080000
SUBDIR+= IQ80321_flash_0xf0080000
SUBDIR+= SMDK2410_flash_0x00000000
SUBDIR+= SMDK2800_flash_0x00000000
SUBDIR+= SMDK2800_flash_0x00100000
SUBDIR+= TS7200_flash_0x60660000
.else
# Big endian platforms
SUBDIR=  ADI_BRH_flash_0x00140000
.endif

.include <bsd.subdir.mk>
