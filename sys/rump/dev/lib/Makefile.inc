#	$NetBSD: Makefile.inc,v 1.3.20.1 2017/12/03 11:39:06 jdolecek Exp $
#

RUMPTOP=	${.CURDIR}/../../..

RUMPDEVLIBDIR:= ${.PARSEDIR}

.if defined(RUMP_COMPONENT) && !empty(RUMP_COMPONENT:Mioconf)
.PATH:	${RUMPDEVLIBDIR}
SRCS+=   component_simple.c
.endif

.include "${RUMPTOP}/Makefile.rump"
