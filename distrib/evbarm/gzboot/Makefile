#	$NetBSD: Makefile,v 1.3 2002/04/13 16:40:32 thorpej Exp $

SUBDIR=
TARGETS+= release

.ifmake !release
# List the Gzip Boot ELF images you want to create here.
SUBDIR+= gzboot_IQ80310_flash_0x00080000
SUBDIR+= gzboot_IQ80321_flash_0xf0080000
.endif

SUBDIR+= .WAIT gzimg

.include <bsd.subdir.mk>
