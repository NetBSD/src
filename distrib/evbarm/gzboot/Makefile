#	$NetBSD: Makefile,v 1.4 2002/04/26 02:31:57 thorpej Exp $

SUBDIR=
TARGETS+= release

.ifmake !release
# List the Gzip Boot ELF images you want to create here.
SUBDIR+= gzboot_IQ80310_flash_0x00080000
SUBDIR+= gzboot_IQ80321_flash_0xf0080000
SUBDIR+= gzboot_TEAMASA_NPWR_flash_0x00080000
.endif

SUBDIR+= .WAIT gzimg

.include <bsd.subdir.mk>
