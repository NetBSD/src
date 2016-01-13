#	$NetBSD: Makefile,v 1.1 2016/01/13 03:39:28 christos Exp $

.include <bsd.init.mk>

DIST=		${IDIST}/src
.PATH:		${DIST} ${IDIST}/doc ${IDIST}/man

PROG=		diff
SRCS=		analyze.c context.c diff.c dir.c ed.c ifdef.c io.c \
		normal.c side.c util.c version.c

INFOFLAGS=	-I${IDIST}/doc
TEXINFO=	diff.texi

COPTS.ifdef.c = -Wno-stack-protector

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-string-plus-int

.include <bsd.info.mk>
.include <bsd.prog.mk>
