#	$NetBSD: Makefile,v 1.4 2005/01/21 05:15:36 rtr Exp $

.include <bsd.own.mk>

LIB=	Xau
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	AuDispose.c AuFileName.c AuGetAddr.c AuGetBest.c AuLock.c \
	AuRead.c AuUnlock.c AuWrite.c

INCS=	Xauth.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	${X11FLAGS.THREADS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
