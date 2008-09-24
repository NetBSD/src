#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:04 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xinerama
.PATH:	${X11SRCDIR.${LIB}}/src

SRCS=	Xinerama.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
