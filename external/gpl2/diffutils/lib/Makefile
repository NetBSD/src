#	$NetBSD: Makefile,v 1.1 2016/01/13 03:39:28 christos Exp $

LIBISPRIVATE=	yes
.include <bsd.init.mk>

LIB=	diffutils

DIST=	${IDIST}/lib
.PATH:	${DIST}

SRCS=	basename.c c-stack.c cmpbuf.c error.c exclude.c exitfail.c \
	freesoft.c \
	hard-locale.c imaxtostr.c offtostr.c prepargs.c posixver.c quotesys.c \
	regex.c setmode.c strftime.c umaxtostr.c xmalloc.c xstrtoumax.c

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-tautological-compare

.include <bsd.lib.mk>
