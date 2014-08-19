#	$NetBSD: Makefile,v 1.2.2.1 2014/08/20 00:00:09 tls Exp $

.include <bsd.own.mk>

PROG=	xvinfo

LDADD+=	-lXv -lXext -lX11
DPADD+=	${LIBXV} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-DPACKAGE_STRING="\"xvinfo 1.1.2\""

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-dangling-else

.include <bsd.x11.mk>
.include <bsd.prog.mk>
