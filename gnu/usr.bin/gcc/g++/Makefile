#	$NetBSD: Makefile,v 1.14 2001/08/19 14:23:55 mrg Exp $

.include <bsd.own.mk>

PROG=           g++
SRCS=		g++spec.c

LINKS+=		${BINDIR}/g++ ${BINDIR}/c++

MAN=		g++.1
MLINKS=		g++.1 c++.1

.include "../Makefile.frontend"

.PATH: ${DIST}/gcc ${DIST}/gcc/cp
