#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

PROG=		xedit
SRCS=		xedit.c commands.c util.c ispell.c options.c hook.c lisp.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/xedit/lisp/re

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

.PATH:		${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
