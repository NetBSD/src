#	$NetBSD: Makefile,v 1.8 2025/02/20 21:20:02 rillig Exp $

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

LINTFLAGS+=	-X 34		# bit-field with underlying enum
LINTFLAGS+=	-X 117		# signed '>>'
LINTFLAGS+=	-X 132		# loss of accuracy
LINTFLAGS+=	-X 220		# fallthrough in 'case' statement
LINTFLAGS+=	-X 231		# unused parameter
LINTFLAGS+=	-X 275		# cast discards 'const'
LINTFLAGS+=	-X 298		# loss of accuracy in function call
LINTFLAGS+=	-X 341		# wrong <ctype.h> usage
LINTFLAGS+=	-X 344		# plain 'int' in bit-field
LINTFLAGS+=	-X 351		# missing header declaration

.include <bsd.x11.mk>
.include <bsd.lib.mk>
