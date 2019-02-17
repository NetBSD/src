# $NetBSD: Makefile,v 1.2 2019/02/17 23:54:08 bad Exp $

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

SRCS=		dnvlist.c msgio.c nvlist.c nvpair.c nv_kern_netbsd.c

.include <bsd.lib.mk>
