#	$NetBSD: Makefile.inc,v 1.8 2016/01/26 23:12:14 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../..

RUMPDEVLIBDIR:= ${.PARSEDIR}

.if defined(RUMP_COMPONENT) && !empty(RUMP_COMPONENT:Mioconf)
.PATH:	${RUMPDEVLIBDIR}
SRCS+=   component_simple.c
.endif

.include "${RUMPTOP}/Makefile.rump"
