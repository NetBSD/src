#	$NetBSD: Makefile,v 1.4.2.2 2016/09/14 03:04:16 pgoyette Exp $

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

LDADD+=	-lrt
DPADD+=	${LIBRT}

NOMAN=	# defined

PKGDIST=	${LIB}

COPTS.xshmfence_semaphore.c += -Wno-stack-protector

.include <bsd.x11.mk>
.include <bsd.lib.mk>
