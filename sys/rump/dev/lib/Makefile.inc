#	$NetBSD: Makefile.inc,v 1.3.38.3 2016/03/19 11:30:33 skrll Exp $
#

RUMPTOP=	${.CURDIR}/../../..

RUMPDEVLIBDIR:= ${.PARSEDIR}

.if defined(RUMP_COMPONENT) && !empty(RUMP_COMPONENT:Mioconf)
.PATH:	${RUMPDEVLIBDIR}
SRCS+=   component_simple.c
.endif

.include "${RUMPTOP}/Makefile.rump"
