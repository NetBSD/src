#	$NetBSD: Makefile.dreamcast.inc,v 1.7 2001/12/09 05:00:43 atatat Exp $

MACHINE_ARCH=sh3el

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
