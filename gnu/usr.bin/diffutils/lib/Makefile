#	$NetBSD: Makefile,v 1.2.24.1 2008/05/18 12:29:49 yamt Exp $

LIBISPRIVATE=	yes

.include "${.CURDIR}/../Makefile.inc"

LIB=	diffutils

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	basename.c c-stack.c cmpbuf.c error.c exclude.c exitfail.c \
	freesoft.c \
	hard-locale.c imaxtostr.c offtostr.c prepargs.c posixver.c quotesys.c \
	regex.c setmode.c strftime.c umaxtostr.c xmalloc.c xstrtoumax.c

.include <bsd.lib.mk>
