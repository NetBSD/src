#	$NetBSD: Makefile.mmeye.inc,v 1.2 2001/01/03 18:15:47 itojun Exp $

MACHINE=mmeye
MACHINE_ARCH=sh3

TEXTADDR=8c010000 
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -Ttext ${TEXTADDR} -e start
.endif
