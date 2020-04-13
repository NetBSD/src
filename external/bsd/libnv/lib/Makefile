# $NetBSD: Makefile,v 1.2.2.3 2020/04/13 07:46:10 martin Exp $

WARNS?=		5

.include <bsd.init.mk>

.if ${RUMPRUN:Uno} != "yes"
LIBISPRIVATE?=	yes
.endif
LIB=		nv
#USE_SHLIBDIR=	yes
#SHLIB_MAJOR=	1
#SHLIB_MINOR=	0

#INCS=		nv.h dnv.h
#INCSDIR=	/usr/include
#MAN=		nv.3

.include "srcs.mk"
SRCS=		${LIBNV_SRCS}

.include <bsd.lib.mk>
