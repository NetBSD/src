#	$NetBSD: Makefile,v 1.1.1.1.8.1 2009/09/17 04:24:28 snj Exp $

.include <bsd.own.mk>

PROG=		xedit
SRCS=		xedit.c commands.c util.c ispell.c options.c hook.c lisp.c \
		tags.c hash.c

CPPFLAGS+=	-I${X11SRCDIR.xedit}/lisp/re

APPDEFS=	Xedit.ad Xedit-color.ad

BUILDSYMLINKS=	Xedit-xprint.ad Xedit.ad

.for _L in lisp mp re
LIB.${_L}!=	cd ${.CURDIR}/../${_L} && ${PRINTOBJDIR}
DPADD+=		${LIB.${_L}}/lib${_L}.a
LDADD+=		-L${LIB.${_L}} -l${_L}
.endfor

LDADD+=		-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=		${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
		${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:		${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults

.include <bsd.x11.mk>
.include <bsd.prog.mk>
