#	$NetBSD: Makefile,v 1.2 2003/08/04 06:58:19 mrg Exp $

.include <bsd.own.mk>

PROG=           cpp
SRCS=		cppspec.c

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS=		${BINDIR}/cpp ${BINDIR}/gcpp

TEXINFO=	cpp.texi
INFOFLAGS=	-I${DIST}/gcc/doc -I${DIST}/gcc/doc/include

MAN=		cpp.1
MLINKS+=	cpp.1 gcpp.1

.include "../Makefile.frontend"

.include <bsd.info.mk>

.PATH: ${DIST}/gcc ${DIST}/gcc/doc
