#	$NetBSD: Makefile.mmeye.inc,v 1.6.24.1 2005/02/06 07:45:25 jmc Exp $

MACHINE_ARCH=	sh3eb
CPPFLAGS+=	-I$S/dist/ipf

TEXTADDR?=8c010000
.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
