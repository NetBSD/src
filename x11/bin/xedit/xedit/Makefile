#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:07 lukem Exp $

.include <bsd.own.mk>

PROG=		xedit
SRCS=		xedit.c commands.c util.c ispell.c options.c hook.c lisp.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/xedit/lisp/re

APPDEFS=	Xedit.ad Xedit-color.ad

.for _L in lisp mp re
LIB.${_L}!=	cd ${.CURDIR}/../${_L} && ${PRINTOBJDIR}
DPADD+=		${LIB.${_L}}/lib${_L}.a
LDADD+=		-L${LIB.${_L}} -l${_L}
.endfor

LDADD+=		-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=		${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
		${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:		${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
