#	$NetBSD: Makefile.dreamcast.inc,v 1.5 2001/10/23 19:55:56 thorpej Exp $

MACHINE_ARCH=sh3el

TEXTADDR=8c010000 
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -Ttext ${TEXTADDR} -e start
.endif
