#	$NetBSD: Makefile.dreamcast.inc,v 1.4.4.1 2002/01/10 19:41:55 thorpej Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
