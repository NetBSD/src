#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	xprop
SRCS=	xprop.c dsimple.c

CPPFLAGS+=-DHAS_WCHAR_H -DHAS_WCTYPE_H -DNO_WIDEC_H
CPPFLAGS+=-I${X11SRCDIR.xlsfonts} -I${X11SRCDIR.${PROG}}/../include
.PATH:	${X11SRCDIR.xlsfonts}

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
