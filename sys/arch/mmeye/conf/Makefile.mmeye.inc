#	$NetBSD: Makefile.mmeye.inc,v 1.3.6.1 2002/01/10 19:46:25 thorpej Exp $

MACHINE_ARCH=sh3eb

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
