#	$NetBSD: Makefile,v 1.3 2001/08/19 14:23:55 mrg Exp $

.include <bsd.own.mk>

PROG=           gcc
SRCS=		gccspec.c

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc 

MAN=		gcc.1
MLINKS=		gcc.1 cc.1

TEXINFO=	gcc.texi
INFOFLAGS=      -I${DIST}/gcc

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc

.include <bsd.info.mk>
