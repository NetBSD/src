#	$NetBSD: Makefile.dreamcast.inc,v 1.3.2.3 2001/03/12 13:28:07 bouyer Exp $

MACHINE=dreamcast
MACHINE_ARCH=sh3el

TEXTADDR=8c010000 
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -Ttext ${TEXTADDR} -e start
.endif
