#	$NetBSD: Makefile.mmeye.inc,v 1.4 2001/10/23 19:57:04 thorpej Exp $

MACHINE_ARCH=sh3eb

TEXTADDR=8c010000 
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -Ttext ${TEXTADDR} -e start
.endif
