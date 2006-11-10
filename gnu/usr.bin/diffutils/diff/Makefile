#	$NetBSD: Makefile,v 1.2 2006/11/10 17:36:08 christos Exp $

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

.include <bsd.info.mk>
.include <bsd.prog.mk>
