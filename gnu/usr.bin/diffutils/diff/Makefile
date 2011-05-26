#	$NetBSD: Makefile,v 1.3 2011/05/26 12:56:27 joerg Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=		${IDIST}/src
.PATH:		${DIST} ${IDIST}/doc ${IDIST}/man

PROG=		diff
SRCS=		analyze.c context.c diff.c dir.c ed.c ifdef.c io.c \
		normal.c side.c util.c version.c

DPADD+=		${LIBDIFFUTILS}
LDADD+=		${LIBDIFFUTILS}

INFOFLAGS=	-I${IDIST}/doc
TEXINFO=	diff.texi

COPTS.ifdef.c = -Wno-stack-protector

CWARNFLAGS.clang+=	-Wno-unused-value

.include <bsd.info.mk>
.include <bsd.prog.mk>
