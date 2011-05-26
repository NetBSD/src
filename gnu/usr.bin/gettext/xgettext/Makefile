#	$NetBSD: Makefile,v 1.5 2011/05/26 12:56:28 joerg Exp $

PROG=	xgettext
SRCS=	\
po-time.c \
uniname.c \
xgettext.c \
x-awk.c \
x-c.c \
x-csharp.c \
x-elisp.c \
x-glade.c \
x-java.c \
x-librep.c \
x-lisp.c \
x-perl.c \
x-php.c \
x-po.c \
x-python.c \
x-rst.c \
x-scheme.c \
x-sh.c \
x-smalltalk.c \
x-tcl.c \
x-ycp.c 

COPTS.xgettext.c = -Wno-stack-protector

CWARNFLAGS.clang+=	-Wno-empty-body

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

.PATH: ${DIST}/gettext/gettext-tools/libuniname
CPPFLAGS+=	-I${DIST}/gettext/gettext-tools/libuniname
