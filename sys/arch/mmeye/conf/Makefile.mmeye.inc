#	$NetBSD: Makefile.mmeye.inc,v 1.5 2001/11/20 12:56:33 lukem Exp $

MACHINE_ARCH=sh3eb

TEXTADDR=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -Ttext ${TEXTADDR} -e start
.endif
