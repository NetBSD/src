#	$NetBSD: Makefile,v 1.6 2024/07/04 00:33:28 mrg Exp $

.include <bsd.own.mk>

PROG=		xmore
APPDEFS=	XMore

SRCS=	xmore.c

CPPFLAGS+=	-DXMORE
CPPFLAGS+=	-DPACKAGE_STRING=\"${PACKAGE_STRING}\"

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} \
	${LIBICE} ${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults ${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
