#	$NetBSD: Makefile,v 1.1.1.1.8.1 2009/09/17 04:24:29 snj Exp $

.include <bsd.own.mk>

PROG=	xmag
SRCS=	xmag.c CutPaste.c RootWin.c Scale.c

APPDEFS=Xmag

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults

.include <bsd.x11.mk>
.include <bsd.prog.mk>
