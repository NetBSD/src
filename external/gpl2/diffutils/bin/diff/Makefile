#	$NetBSD: Makefile,v 1.2 2019/11/11 22:45:26 joerg Exp $

.include <bsd.init.mk>

DIST=		${IDIST}/src
.PATH:		${DIST} ${IDIST}/doc ${IDIST}/man

PROG=		diff
SRCS=		analyze.c context.c diff.c dir.c ed.c ifdef.c io.c \
		normal.c side.c util.c version.c

INFOFLAGS=	-I${IDIST}/doc
TEXINFO=	diff.texi

COPTS.ifdef.c = -Wno-stack-protector

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-string-plus-int -Wno-error=bool-operation

.include <bsd.info.mk>
.include <bsd.prog.mk>
