#	$NetBSD: Makefile,v 1.3.12.1 2013/06/23 06:28:41 tls Exp $

.include <bsd.own.mk>

PROG=		xedit
SRCS=		xedit.c commands.c util.c ispell.c options.c hook.c lisp.c \
		tags.c hash.c

CPPFLAGS+=	-I${X11SRCDIR.xedit}/lisp/re

APPDEFS=	Xedit Xedit-color

.for _L in lisp mp re
LIB.${_L}!=	cd ${.CURDIR}/../${_L} && ${PRINTOBJDIR}
DPADD+=		${LIB.${_L}}/lib${_L}.a
LDADD+=		-L${LIB.${_L}} -l${_L}
.endfor

LDADD+=		-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=		${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
		${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
