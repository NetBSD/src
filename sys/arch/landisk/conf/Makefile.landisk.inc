#	$NetBSD: Makefile.landisk.inc,v 1.1.4.1 2007/02/01 08:48:04 ad Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=0x8c001000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
