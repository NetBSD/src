#	$NetBSD: Makefile,v 1.2 2001/07/18 14:00:12 tv Exp $

.include <bsd.own.mk>

PROG=           gcc
SRCS=		gccspec.c

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc 

MAN=		gcc.1
MLINKS=		gcc.1 cc.1

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc
