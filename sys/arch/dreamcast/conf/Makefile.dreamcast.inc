#	$NetBSD: Makefile.dreamcast.inc,v 1.8.6.1 2005/12/11 10:28:16 christos Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
