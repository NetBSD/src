#	$NetBSD: Makefile,v 1.3 2010/11/21 02:33:42 mrg Exp $

.include <bsd.own.mk>

PROG=	xprop
SRCS=	xprop.c dsimple.c clientwin.c

CPPFLAGS+=-DHAVE_WCHAR_H -DHAVE_WCTYPE_H -DNO_WIDEC_H
CPPFLAGS+=-I${X11SRCDIR.${PROG}}/../include

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
