#	$NetBSD: Makefile,v 1.16 2003/09/02 05:23:06 mrg Exp $

.include <bsd.own.mk>

PROG=           cpp
SRCS=		cppspec.c

LINKS=		${BINDIR}/cpp ${BINDIR}/gcpp

TEXINFO=	cpp.texi

MAN=		cpp.1
MLINKS=		cpp.1 gcpp.1 cpp.1 cpp.1

# delete "cccp" man page before netbsd 2.0 if you remember...
CLEANFILES+=	cpp.1
cpp.1: ${DIST}/gcc/cccp.1
	rm -f cpp.1
	cp ${DIST}/gcc/cccp.1 cpp.1

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
