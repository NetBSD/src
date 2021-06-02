#	$NetBSD: Makefile,v 1.5.24.1 2021/06/02 14:46:20 cjep Exp $

.include <bsd.own.mk>

PROG=		xedit
SRCS=		xedit.c commands.c util.c ispell.c options.c hook.c lisp.c \
		tags.c hash.c

CPPFLAGS+=	-I${X11SRCDIR.xedit}/lisp/re
CPPFLAGS+=	-DHAVE_GETPAGESIZE

APPDEFS=	Xedit Xedit-color

.for _L in lisp mp re
LIB.${_L}!=	cd ${.CURDIR}/../${_L} && ${PRINTOBJDIR}
DPADD+=		${LIB.${_L}}/lib${_L}.a
LDADD+=		-L${LIB.${_L}} -l${_L}
.endfor

LDADD+=		${XLIBLDADD_XAW} -lm
DPADD+=		${XLIBDPADD_XAW} ${LIBM}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/app-defaults
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
