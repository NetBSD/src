#	$NetBSD: Makefile.inc,v 1.5 2015/11/11 21:52:45 pooka Exp $
#

RUMPTOP=	${.CURDIR}/../../..
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern -I${RUMPTOP}/librump/rumpdev

RUMPDEVLIBDIR:= ${.PARSEDIR}
.ifdef COMPONENT_SIMPLE
.PATH:	${RUMPDEVLIBDIR}
SRCS+=   component_simple.c
CPPFLAGS.component_simple.c+= -DCOMPONENT_NAME=${COMPONENT_SIMPLE}
.endif

.include "${RUMPTOP}/Makefile.rump"
