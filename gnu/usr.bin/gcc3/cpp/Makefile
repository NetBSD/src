#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:33 mrg Exp $

.include <bsd.own.mk>

PROG=           cpp
SRCS=		cppspec.c

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS=		${BINDIR}/cpp ${BINDIR}/gcpp

TEXINFO=	cpp.texi
INFOFLAGS=	-I${DIST}/gcc/doc -I${DIST}/gcc/doc/include

#MAN=		ccp.1
#MLINKS=		ccp.1 gcpp.1
MKMAN=no

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc ${DIST}/gcc/doc

.include <bsd.info.mk>
