#	$NetBSD: Makefile,v 1.1.1.1 2003/01/26 00:40:40 wiz Exp $

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

.include <bsd.info.mk>
.include <bsd.prog.mk>
