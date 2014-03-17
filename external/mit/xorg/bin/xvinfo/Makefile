#	$NetBSD: Makefile,v 1.3 2014/03/17 10:09:18 mrg Exp $

.include <bsd.own.mk>

PROG=	xvinfo

LDADD+=	-lXv -lXext -lX11
DPADD+=	${LIBXV} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-DPACKAGE_STRING=\"1.1.2\"

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-dangling-else

.include <bsd.x11.mk>
.include <bsd.prog.mk>
