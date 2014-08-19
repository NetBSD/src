#	$NetBSD: Makefile,v 1.1.8.2 2014/08/20 00:00:12 tls Exp $

.include <bsd.own.mk>

LIB=	pthread-stubs
.PATH:	${X11SRCDIR.${LIB}}

SRCS=	stubs.c

CPPFLAGS+=	${X11FLAGS.THREADLIB} -I${X11SRCDIR.${LIB}}/../include

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
