#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:45 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	Xau
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	AuDispose.c AuFileName.c AuGetAddr.c AuGetBest.c AuLock.c \
	AuRead.c AuUnlock.c AuWrite.c

INCS=	Xauth.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
