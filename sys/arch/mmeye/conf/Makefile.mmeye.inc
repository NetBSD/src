#	$NetBSD: Makefile.mmeye.inc,v 1.6.12.2 2001/12/09 05:00:48 atatat Exp $

MACHINE_ARCH=sh3eb

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
