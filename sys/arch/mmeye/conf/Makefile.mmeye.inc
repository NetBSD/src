#	$NetBSD: Makefile.mmeye.inc,v 1.3 2001/01/04 07:03:22 itojun Exp $

MACHINE=mmeye
MACHINE_ARCH=sh3eb

TEXTADDR=8c010000 
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -Ttext ${TEXTADDR} -e start
.endif
