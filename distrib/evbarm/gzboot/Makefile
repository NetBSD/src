#	$NetBSD: Makefile,v 1.5 2003/04/10 01:50:28 thorpej Exp $

SUBDIR=
TARGETS+= release

.ifmake !release
# List the Gzip Boot ELF images you want to create here.
SUBDIR+= gzboot_ADI_BRH_flash_0x00140000
SUBDIR+= gzboot_IQ80310_flash_0x00080000
SUBDIR+= gzboot_IQ80321_flash_0xf0080000
SUBDIR+= gzboot_TEAMASA_NPWR_flash_0x00080000
.endif

SUBDIR+= .WAIT gzimg

.include <bsd.subdir.mk>
