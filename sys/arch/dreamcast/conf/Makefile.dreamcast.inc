#	$NetBSD: Makefile.dreamcast.inc,v 1.4 2001/02/14 18:06:50 msaitoh Exp $

MACHINE=dreamcast
MACHINE_ARCH=sh3el

TEXTADDR=8c010000 
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -Ttext ${TEXTADDR} -e start
.endif
