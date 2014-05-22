#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 14:09:43 yamt Exp $

.include <bsd.own.mk>

LIB=	pthread-stubs
.PATH:	${X11SRCDIR.${LIB}}

SRCS=	stubs.c

CPPFLAGS+=	${X11FLAGS.THREADLIB} -I${X11SRCDIR.${LIB}}/../include

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
