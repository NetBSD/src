#	$NetBSD: Makefile,v 1.3 2003/10/20 05:12:41 jdolecek Exp $

.include <bsd.own.mk>

LIB=	Xinerama
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Xinerama.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
