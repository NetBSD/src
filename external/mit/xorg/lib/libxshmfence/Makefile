#	$NetBSD: Makefile,v 1.1 2016/08/14 00:02:19 mrg Exp $

.include <bsd.own.mk>

LIB=	xshmfence

XSHMFENCEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XSHMFENCEDIR}/src
SRCS=		xshmfence_pthread.c xshmfence_alloc.c

INCS=	xshmfence.h
INCSDIR=${X11INCDIR}/X11

#CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-DHAVE_PTHREAD
CPPFLAGS+=	-DSHMDIR=\"/var/shm\"
# XXX
CPPFLAGS+=	-D_PTHREAD_PSHARED

NOMAN=	# defined

#LIBDPLIBS=	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
