#	$NetBSD: Makefile,v 1.2 2004/05/23 02:24:51 lukem Exp $

MKPRIVATELIB=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	greputils

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	closeout.c error.c exclude.c hard-locale.c isdir.c \
	obstack.c quotearg.c regex.c savedir.c stpcpy.c \
	xmalloc.c xstrtol.c xstrtoumax.c

.include <bsd.lib.mk>
