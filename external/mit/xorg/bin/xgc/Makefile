#	$NetBSD: Makefile,v 1.3.12.1 2013/06/23 06:28:41 tls Exp $

.include <bsd.own.mk>

PROG=	xgc
SRCS=	dashlist.c planemask.c getfile.c tests.c text.c \
	choice.c main.c interpret.c record.c testfrac.c gram.y lex.l

YHEADER=1
CPPFLAGS+=-I. -I${X11SRCDIR.${PROG}}

APPDEFS=Xgc Xgc-color

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXPM} \
	${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
