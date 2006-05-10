#	$NetBSD: Makefile,v 1.1 2006/05/10 22:48:12 mrg Exp $

.include <bsd.own.mk>

PROG=		cpp
SRCS=		cppspec.c

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS=		${BINDIR}/cpp ${BINDIR}/gcpp

TEXINFO=	cpp.texi
INFOFLAGS=	-I${DIST}/gcc/doc -I${DIST}/gcc/doc/include -I.

MAN=		cpp.1
MLINKS+=	cpp.1 gcpp.1 cpp.1 cccp.1

.include "../Makefile.frontend"

.include <bsd.info.mk>

.PATH: ${DIST}/gcc ${DIST}/gcc/doc

cpp.info: gcc-vers.texi
