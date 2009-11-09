#	$NetBSD: Makefile,v 1.2 2009/11/09 00:30:27 mrg Exp $

.include <bsd.own.mk>

PROG=	xprop
SRCS=	xprop.c dsimple.c clientwin.c

CPPFLAGS+=-DHAS_WCHAR_H -DHAS_WCTYPE_H -DNO_WIDEC_H
CPPFLAGS+=-I${X11SRCDIR.${PROG}}/../include

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
