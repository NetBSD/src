#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:37 mrg Exp $

.include <bsd.own.mk>

PROG=           gcc
SRCS=		gccspec.c

CPPFLAGS+=	-I${BACKENDOBJ}

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc 

MAN=		gcc.1
MLINKS=		gcc.1 cc.1

TEXINFO=	gcc.texi
INFOFLAGS=      -I${DIST}/gcc/doc -I${DIST}/gcc/doc/include

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc ${DIST}/gcc/doc

.include <bsd.info.mk>
