#	$NetBSD: Makefile,v 1.14 2001/08/19 14:23:55 mrg Exp $

.include <bsd.own.mk>

PROG=           cpp
SRCS=		cppspec.c

LINKS=		${BINDIR}/cpp ${BINDIR}/gcpp

TEXINFO=	cpp.texi

MAN=		cccp.1
MLINKS=		cccp.1 gcpp.1


.include "../Makefile.frontend"

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
