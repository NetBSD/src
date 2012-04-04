#	$NetBSD: Makefile,v 1.4 2012/04/04 10:59:46 joerg Exp $

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

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-string-plus-int

.include <bsd.info.mk>
.include <bsd.prog.mk>
