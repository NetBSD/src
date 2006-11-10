#	$NetBSD: Makefile,v 1.3 2006/11/10 17:36:09 christos Exp $

MKPRIVATELIB=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	greputils

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	closeout.c error.c exclude.c hard-locale.c isdir.c \
	obstack.c quotearg.c regex.c savedir.c stpcpy.c \
	xmalloc.c xstrtol.c xstrtoumax.c

COPTS.hard-locale.c = -Wno-stack-protector
COPTS.regex.c = -Wno-stack-protector

.include <bsd.lib.mk>
