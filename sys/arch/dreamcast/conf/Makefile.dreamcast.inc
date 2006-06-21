#	$NetBSD: Makefile.dreamcast.inc,v 1.8.22.1 2006/06/21 14:50:31 yamt Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
