#	$NetBSD: Makefile,v 1.2 2016/08/14 03:43:03 mrg Exp $

.include <bsd.own.mk>

LIB=	xshmfence

XSHMFENCEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XSHMFENCEDIR}/src
SRCS=	 	xshmfence_alloc.c xshmfence_semaphore.c

INCS=	xshmfence.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	-DSHMDIR=\"/var/shm\"
CPPFLAGS+=	-DHAVE_SEMAPHORE
# XXX
CPPFLAGS+=	-DLIBXSHM_PAGESIZE=4096

NOMAN=	# defined

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
