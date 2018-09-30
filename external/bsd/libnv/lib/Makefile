# $NetBSD: Makefile,v 1.1.2.2 2018/09/30 01:45:12 pgoyette Exp $

LIBISPRIVATE=	yes
LIB=		nv
#USE_SHLIBDIR=	yes
#SHLIB_MAJOR=	1
#SHLIB_MINOR=	0

WARNS?=		5

.include <bsd.init.mk>

#INCS=		nv.h dnv.h
#INCSDIR=	/usr/include
#MAN=		nv.3

SRCS=		dnvlist.c msgio.c nvlist.c nvpair.c nv_kern_netbsd.c

.include <bsd.lib.mk>
