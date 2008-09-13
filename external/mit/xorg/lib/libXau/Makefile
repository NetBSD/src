#	$NetBSD: Makefile,v 1.2 2008/09/13 04:32:30 cube Exp $

.include <bsd.own.mk>

LIB=	Xau
.PATH:	${X11SRCDIR.${LIB}}
.PATH:	${X11SRCDIR.${LIB}}/include/X11

SRCS=	AuDispose.c AuFileName.c AuGetAddr.c AuGetBest.c AuLock.c \
	AuRead.c AuUnlock.c AuWrite.c

INCS=	Xauth.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	${X11FLAGS.THREADLIB}

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
