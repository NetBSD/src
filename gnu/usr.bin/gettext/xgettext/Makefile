#	$NetBSD: Makefile,v 1.4 2006/11/10 17:36:09 christos Exp $

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

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

.PATH: ${DIST}/gettext/gettext-tools/libuniname
CPPFLAGS+=	-I${DIST}/gettext/gettext-tools/libuniname
