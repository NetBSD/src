#	$NetBSD: Makefile.mmeye.inc,v 1.3.2.2 2001/01/05 17:34:48 bouyer Exp $

MACHINE=mmeye
MACHINE_ARCH=sh3eb

TEXTADDR=8c010000 
.if ${OBJECT_FMT} == "ELF"
LINKFLAGS=	-N -Ttext ${TEXTADDR} -e start
.endif
