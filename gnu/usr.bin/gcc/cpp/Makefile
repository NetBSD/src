#	$NetBSD: Makefile,v 1.19 2003/09/04 04:17:01 mrg Exp $

.include <bsd.own.mk>

PROG=           cpp
SRCS=		cppspec.c

LINKS=		${BINDIR}/cpp ${BINDIR}/gcpp

TEXINFO=	cpp.texi

MAN=		cccp.1
MLINKS=		cccp.1 gcpp.1 cccp.1 cpp.1


.include "../Makefile.frontend"

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
