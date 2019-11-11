#	$NetBSD: Makefile,v 1.7 2019/11/11 22:45:26 joerg Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=		lisp
SRCS=		bytecode.c core.c debugger.c format.c hash.c helper.c \
		io.c lisp.c math.c package.c pathname.c read.c \
		regex.c require.c stream.c string.c struct.c time.c \
		write.c xedit.c

CPPFLAGS+=	-DLISP  -DLISPDIR=\"${X11LIBDIR}/xedit/lisp\" \
		-I${X11SRCDIR.xedit}/lisp/mp \
		-I${X11SRCDIR.xedit}/lisp/re \
		-I${X11SRCDIR.xedit}

.if defined(HAVE_PCC)
# code uses gcc-specific "label as values" feature
CPPFLAGS+=	-DANSI_SOURCE
.endif

.PATH:		${X11SRCDIR.xedit}/lisp

CWARNFLAGS.clang+=	-Wno-error=implicit-int-float-conversion

# Ugh. writes to const char *
COPTS.format.c+=	-Wno-error
COPTS.lisp.c+=		-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
