#	$NetBSD: Makefile.dreamcast.inc,v 1.8.28.1 2005/11/29 21:22:57 yamt Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
