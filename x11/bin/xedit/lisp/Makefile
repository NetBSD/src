#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:17 lukem Exp $

.include <bsd.own.mk>

LIB=		lisp
SRCS=		bytecode.c core.c debugger.c format.c hash.c helper.c \
		io.c lisp.c math.c package.c pathname.c read.c \
		regex.c require.c stream.c string.c struct.c time.c \
		write.c xedit.c

CPPFLAGS+=	-DLISP  -DLISPDIR=\"${X11LIBDIR}/xedit/lisp\" \
		-I${X11SRCDIR.xc}/programs/xedit/lisp/mp \
		-I${X11SRCDIR.xc}/programs/xedit/lisp/re

.PATH:		${X11SRCDIR.xc}/programs/xedit/lisp

.include <bsd.x11.mk>
.include <bsd.lib.mk>
