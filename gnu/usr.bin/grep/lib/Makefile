#	$NetBSD: Makefile,v 1.5 2011/05/26 12:56:28 joerg Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	greputils

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	closeout.c error.c exclude.c hard-locale.c isdir.c \
	obstack.c quotearg.c regex.c savedir.c stpcpy.c \
	xmalloc.c xstrtol.c xstrtoumax.c

COPTS.hard-locale.c = -Wno-stack-protector
COPTS.regex.c = -Wno-stack-protector

CWARNFLAGS.clang+=	-Wno-unused-value

.include <bsd.lib.mk>
