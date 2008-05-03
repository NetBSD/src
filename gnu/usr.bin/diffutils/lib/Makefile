#	$NetBSD: Makefile,v 1.3 2008/05/03 14:48:30 lukem Exp $

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
