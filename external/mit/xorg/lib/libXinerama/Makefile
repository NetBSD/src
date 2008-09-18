#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:37 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xinerama
.PATH:	${X11SRCDIR.${LIB}}/src

SRCS=	Xinerama.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
