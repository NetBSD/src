#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:50:20 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	Xau
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	AuDispose.c AuFileName.c AuGetAddr.c AuGetBest.c AuLock.c \
	AuRead.c AuUnlock.c AuWrite.c

INCS=	Xauth.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
