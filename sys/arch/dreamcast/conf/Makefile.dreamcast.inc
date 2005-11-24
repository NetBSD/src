#	$NetBSD: Makefile.dreamcast.inc,v 1.9 2005/11/24 12:54:29 dbj Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
