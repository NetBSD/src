#	$NetBSD: Makefile,v 1.5.4.1 2026/11/04 23:15:24 perseant Exp $

.include <bsd.own.mk>

PROG=	xgc
SRCS=	dashlist.c planemask.c getfile.c tests.c text.c \
	choice.c main.c interpret.c record.c testfrac.c gram.y lex.l

YHEADER=1
#CPPFLAGS+=-D_CONST_X_STRING
CPPFLAGS+=-DHAVE_LRAND48
CPPFLAGS+=-I. -I${X11SRCDIR.${PROG}}

APPDEFS=Xgc Xgc-color

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXPM} \
	${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
