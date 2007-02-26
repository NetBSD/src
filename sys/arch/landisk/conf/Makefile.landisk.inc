#	$NetBSD: Makefile.landisk.inc,v 1.1.12.3 2007/02/26 09:07:08 yamt Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=0x8c001000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
