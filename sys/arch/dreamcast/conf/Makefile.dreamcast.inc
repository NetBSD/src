#	$NetBSD: Makefile.dreamcast.inc,v 1.6 2001/11/20 12:56:25 lukem Exp $

MACHINE_ARCH=sh3el

TEXTADDR=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -Ttext ${TEXTADDR} -e start
.endif
