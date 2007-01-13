#	$NetBSD: Makefile.landisk.inc,v 1.2 2007/01/13 22:53:54 freza Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=0x8c001000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
